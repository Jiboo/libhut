/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste Lepesme github.com/jiboo/libhut
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "hut/offscreen.hpp"

#include <algorithm>
#include <utility>

#include "hut/utils/math.hpp"
#include "hut/utils/profiling.hpp"

#include "hut/display.hpp"
#include "hut/image.hpp"

using namespace hut;

offscreen::offscreen(const shared_image &_target, offscreen_params _init_params)
    : render_target(*_target->display_)
    , target_(_target)
    , params_(std::move(_init_params)) {
  HUT_PROFILE_SCOPE(PWINDOW, "offscreen::offscreen");

  if (params_.subres_.coords_ == u16vec4{0, 0, 0, 0})
    params_.subres_.coords_ = make_bbox_with_origin_size(u16vec2{0, 0}, target_->size());

  render_target_params pass_params;
  pass_params.clear_color_         = render_target_params_.clear_color_;
  pass_params.clear_depth_stencil_ = render_target_params_.clear_depth_stencil_;
  pass_params.box_                 = params_.subres_.coords_;
  pass_params.format_              = _target->params_.format_;
  if (params_.flags_ & offscreen_params::FMULTISAMPLING)
    pass_params.flags_ |= render_target_params::FMULTISAMPLING;
  if (params_.flags_ & offscreen_params::FDEPTH)
    pass_params.flags_ |= render_target_params::FDEPTH;
  pass_params.initial_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
  pass_params.final_layout_   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  reinit_pass(pass_params, std::span<VkImageView>{&_target->view_, 1});

  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount          = 1;
  alloc_info.commandPool                 = display_->commandg_pool_;
  if (HUT_PVK(vkAllocateCommandBuffers, display_->device_, &alloc_info, &cb_) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");

  VkFenceCreateInfo fence_info = {};
  fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (HUT_PVK(vkCreateFence, display_->device_, &fence_info, nullptr, &fence_) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate fence!");
}

offscreen::~offscreen() {
  HUT_PROFILE_SCOPE(PWINDOW, "offscreen::~offscreen");

  if (cb_ != VK_NULL_HANDLE)
    HUT_PVK(vkFreeCommandBuffers, display_->device_, display_->commandg_pool_, 1, &cb_);
  if (fence_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroyFence, display_->device_, fence_, nullptr);
}

void offscreen::flush_cb() {
  VkSubmitInfo submit_info       = {};
  submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &cb_;

  if (HUT_PVK(vkQueueSubmit, display_->queueg_, 1, &submit_info, fence_) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer!");

  if (HUT_PVK(vkWaitForFences, display_->device_, 1, &fence_, VK_TRUE, 10ull * 1000 * 1000 * 1000) != VK_SUCCESS)  // 10s timeout
    throw std::runtime_error("error (or time out) while waiting for offscreen render");

  HUT_PVK(vkResetFences, display_->device_, 1, &fence_);
}

void offscreen::draw(const draw_callback &_callback) {
  VkImageSubresourceRange subres_range;
  subres_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  subres_range.baseMipLevel   = params_.subres_.level_;
  subres_range.levelCount     = 1;
  subres_range.baseArrayLayer = params_.subres_.layer_;
  subres_range.layerCount     = 1;

  begin_rebuild_cb(fbos_[0], cb_);
  _callback(cb_);
  end_rebuild_cb(cb_);
  flush_cb();
}

void offscreen::download(std::span<u8> _dst, uint _data_row_pitch, image::subresource _src) {
  // Downloading whole image seems more suitable
  if (_src.coords_ == u16vec4{0, 0, 0, 0})
    _src.coords_ = make_bbox_with_origin_size({0, 0}, target_->size());

  const auto size   = bbox_size(_src.coords_);
  const auto origin = bbox_origin(_src.coords_);

  assert(_src.coords_.z <= target_->size().x);
  assert(_src.coords_.w <= target_->size().y);

  const auto &limits       = display_->device_props_.limits;
  const uint  buffer_align = limits.optimalBufferCopyRowPitchAlignment;
  const uint  offset_align = std::max(VkDeviceSize(4), limits.optimalBufferCopyOffsetAlignment);
  const uint  row_bit_size = size.x * target_->bpp();
  assert(row_bit_size >= 8);
  const uint row_byte_size    = row_bit_size / 8;
  const uint buffer_row_pitch = align(row_byte_size, buffer_align);
  const auto byte_size        = size.y * buffer_row_pitch;

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  begin_info.pInheritanceInfo         = nullptr;  // Optional

  VkImageSubresourceRange subres_range;
  subres_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  subres_range.baseMipLevel   = _src.level_;
  subres_range.levelCount     = 1;
  subres_range.baseArrayLayer = _src.layer_;
  subres_range.layerCount     = 1;

  VkImageSubresourceLayers subres_layers = {};
  subres_layers.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
  subres_layers.mipLevel                 = _src.level_;
  subres_layers.baseArrayLayer           = _src.layer_;
  subres_layers.layerCount               = 1;

  VkBufferImageCopy region;
  region.imageExtent       = {(uint)size.x, (uint)size.y, 1};
  region.imageOffset       = {origin.x, origin.y, 0};
  region.bufferRowLength   = (buffer_row_pitch * 8) / target_->bpp();
  region.bufferImageHeight = size.y;
  region.imageSubresource  = subres_layers;

  std::lock_guard lk(display_->staging_mutex_);
  auto            staging_alloc = display_->staging_->allocate_raw(byte_size, offset_align);
  region.bufferOffset           = staging_alloc->offset_bytes();

  HUT_PVK(vkBeginCommandBuffer, cb_, &begin_info);

  display::transition_image(cb_, target_->image_, subres_range, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  vkCmdCopyImageToBuffer(cb_, target_->image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging_alloc->underlying_buffer(), 1, &region);
  display::transition_image(cb_, target_->image_, subres_range, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  HUT_PVK(vkEndCommandBuffer, cb_);
  flush_cb();

  std::span<u8> staging_data{staging_alloc->existing_mapping(), byte_size};
  if (_data_row_pitch == buffer_row_pitch) {
    assert(_dst.size_bytes() >= staging_data.size_bytes());
    memcpy(_dst.data(), staging_data.data(), staging_data.size());
  } else {
    assert(_dst.size_bytes() >= (size.y * _data_row_pitch));
    for (uint y = 0; y < size.y; y++) {
      auto *src_row = staging_data.data() + y * buffer_row_pitch;
      auto *dst_row = _dst.data() + y * _data_row_pitch;
      memcpy(dst_row, src_row, row_byte_size);
    }
  }
}
