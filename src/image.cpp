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

#include "hut/image.hpp"

#include <cstddef>
#include <cstring>

#include "hut/utils/format.hpp"
#include "hut/utils/profiling.hpp"
#include "hut/utils/vulkan.hpp"

#include "hut/display.hpp"

namespace hut {

std::shared_ptr<image> image::load_raw(display &_display, const shared_buffer &_storage, std::span<const u8> _data,
                                       uint _data_row_pitch, const image_params &_params) {
  HUT_PROFILE_FUN(PIMAGE)
  assert(_params.levels_ == 1);
  assert(_params.layers_ == 1);

  auto dst = std::make_shared<image>(_display, _storage, _params);
  dst->update({u16bbox_px{0, 0, _params.size_}}, _data, _data_row_pitch);

  return dst;
}

image::~image() {
  HUT_PROFILE_FUN(PIMAGE)
  auto *const device = display_->device();
  HUT_PVK(vkDestroyImageView, device, view_, nullptr);
  HUT_PVK(vkDestroyImage, device, image_, nullptr);
}

image::image(display &_display, const shared_buffer &_storage, const image_params &_params)
    : display_(&_display)
    , params_(_params) {
  HUT_PROFILE_FUN(PIMAGE)

  const auto max_bounds = u16vec2_px{u16_px{_display.limits().maxImageDimension2D}};
  if (params_.size_ == u16vec2_px{0, 0}) {
    params_.size_ = max_bounds;
  }
  params_.size_ = min(max_bounds, params_.size_);

  assert(params_.size_.x > 0_px);
  assert(params_.size_.y > 0_px);

  VkImageCreateInfo image_info = {};
  image_info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType         = VK_IMAGE_TYPE_2D;
  image_info.extent.width      = (uint)_params.size_.x;
  image_info.extent.height     = (uint)_params.size_.y;
  image_info.extent.depth      = 1;
  image_info.mipLevels         = _params.levels_;
  image_info.arrayLayers       = _params.layers_;
  image_info.format            = _params.format_;
  image_info.tiling            = _params.tiling_;
  image_info.initialLayout     = VK_IMAGE_LAYOUT_PREINITIALIZED;
  image_info.usage             = _params.usage_;
  image_info.samples           = _params.samples_;
  image_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
  image_info.flags             = _params.flags_;

  HUT_VVK(HUT_PVK(vkCreateImage, _display.device(), &image_info, nullptr, &image_));

  VkMemoryRequirements mem_req;
  HUT_PVK(vkGetImageMemoryRequirements, _display.device(), image_, &mem_req);

  // FIXME: Assert that mem_req.memoryTypeBits is compatible with _storage
  alloc_ = _storage->allocate<u8>(mem_req.size, mem_req.alignment);

  HUT_PVK(vkBindImageMemory, _display.device(), image_, alloc_->parent()->memory_, alloc_->offset_bytes());

  const bool            cubemap             = (params_.flags_ & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) != 0u;
  VkImageViewCreateInfo view_info           = {};
  view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image                           = image_;
  view_info.viewType                        = cubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
  view_info.format                          = _params.format_;
  view_info.subresourceRange.aspectMask     = _params.aspect_;
  view_info.subresourceRange.baseMipLevel   = 0;
  view_info.subresourceRange.levelCount     = _params.levels_;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount     = cubemap ? 6 : 1;

  HUT_VVK(HUT_PVK(vkCreateImageView, _display.device(), &view_info, nullptr, &view_));

  if ((_params.usage_ & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
    return;

  VkImageSubresourceRange range;
  range.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
  range.baseMipLevel             = 0;
  range.levelCount               = _params.levels_;
  range.baseArrayLayer           = 0;
  range.layerCount               = _params.layers_;
  display::image_transition pre  = {};
  pre.destination_               = image_;
  pre.old_layout_                = VK_IMAGE_LAYOUT_PREINITIALIZED;
  pre.new_layout_                = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  display::image_transition post = {};
  post.destination_              = image_;
  post.old_layout_               = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  post.new_layout_               = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  display::image_clear clear     = {};
  clear.destination_             = image_;
  clear.color_                   = {};

  std::lock_guard lk(display_->staging_mutex_);
  display_->staging_jobs_++;
  display_->preflush([this, pre, post, clear, range]() {
    display_->stage_transition(pre, range);
    if ((params_.tiling_ & VK_IMAGE_TILING_LINEAR) != 0)  // FIXME
      display_->stage_clear(clear, range);
    display_->stage_transition(post, range);
    display_->staging_jobs_--;
  });
}

void image::update(subresource _subres, std::span<const u8> _data, uint _src_row_pitch) {
  HUT_PROFILE_FUN(PIMAGE, _subres.coords_, _subres.level_, _subres.layer_)
  auto updto = update(_subres);
  auto size  = _subres.coords_.size();
  assert(_data.size_bytes() == static_cast<size_t>(_src_row_pitch * (uint)size.y));
  auto mip_size = params_.size_ >> _subres.level_;
  assert(mip_size.x >= size.x && mip_size.y >= size.y);

  if (_src_row_pitch == updto.staging_row_pitch() || params_.tiling_ == VK_IMAGE_TILING_OPTIMAL) {
    memcpy(updto.data(), _data.data(), _data.size_bytes());
  } else {
    uint row_bit_size = (uint)size.x * bpp();
    assert(row_bit_size >= 8);
    uint row_byte_size = row_bit_size / 8;
    for (uint y = 0; y < (uint)size.y; y++) {
      auto       *dst_row = updto.data() + static_cast<size_t>(y * updto.staging_row_pitch());
      const auto *src_row = _data.data() + static_cast<size_t>(y * _src_row_pitch);
      memcpy(dst_row, src_row, row_byte_size);
    }
  }
}

image::updator image::update(subresource _subres) {
  HUT_PROFILE_FUN(PIMAGE, _subres.coords_, _subres.level_, _subres.layer_)
  auto        size         = _subres.coords_.size();
  const auto &limits       = display_->limits();
  uint        buffer_align = limits.optimalBufferCopyRowPitchAlignment;
  uint        offset_align = std::max(VkDeviceSize(4), limits.optimalBufferCopyOffsetAlignment);
  uint        row_bit_size = (uint)size.x * bpp();
  assert(row_bit_size >= 8);
  uint row_byte_size     = row_bit_size / 8;
  uint staging_row_pitch = align(row_byte_size, buffer_align);
  auto byte_size         = (uint)size.y * staging_row_pitch;

  std::lock_guard lk(display_->staging_mutex_);

  auto staging_alloc = display_->staging_->allocate_raw(byte_size, offset_align);
  auto span          = std::span<u8>{staging_alloc.parent()->permanent_map_ + staging_alloc.offset_bytes(), byte_size};
  return {*this, std::move(staging_alloc), span, staging_row_pitch, _subres};
}

void image::finalize_update(image::updator &_update) {
  HUT_PROFILE_FUN(PIMAGE, _update.subres_.coords_, _update.subres_.level_, _update.subres_.layer_)
  auto size   = _update.subres_.coords_.size();
  auto origin = _update.subres_.coords_.origin();

  VkImageSubresourceLayers subres_layers = {};
  subres_layers.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
  subres_layers.mipLevel                 = _update.subres_.level_;
  subres_layers.baseArrayLayer           = _update.subres_.layer_;
  subres_layers.layerCount               = 1;

  VkImageSubresourceRange subres_range = {};
  subres_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
  subres_range.baseMipLevel            = _update.subres_.level_;
  subres_range.levelCount              = 1;
  subres_range.baseArrayLayer          = _update.subres_.layer_;
  subres_range.layerCount              = 1;

  display::image_transition pre = {};
  pre.destination_              = image_;
  pre.old_layout_               = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  pre.new_layout_               = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

  display::image_transition post = {};
  post.destination_              = image_;
  post.old_layout_               = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  post.new_layout_               = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  display::buffer_image_copy copy = {};
  copy.imageExtent                = {(uint)size.x, (uint)size.y, 1};
  copy.imageOffset                = {(int)origin.x, (int)origin.y, 0};
  copy.bufferRowLength   = params_.tiling_ == VK_IMAGE_TILING_OPTIMAL ? 0 : ((_update.staging_row_pitch_ * 8) / bpp());
  copy.bufferImageHeight = params_.tiling_ == VK_IMAGE_TILING_OPTIMAL ? 0 : (uint)size.y;
  copy.bufferOffset      = _update.staging_.offset_bytes();
  copy.imageSubresource  = subres_layers;
  copy.source_           = _update.staging_.parent()->buffer_;
  copy.destination_      = image_;
  copy.bytes_size_       = _update.size_bytes();

  std::lock_guard lk(display_->staging_mutex_);
  display_->staging_jobs_++;
  display_->preflush([this, pre, post, copy, subres_range]() {
    display_->stage_transition(pre, subres_range);
    display_->stage_copy(copy);
    display_->stage_transition(post, subres_range);
    display_->staging_jobs_--;
  });
  display_->postflush_collect(std::move(_update.staging_));
}

}  // namespace hut
