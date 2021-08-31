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

#include <cstring>

#include <iostream>

#include "hut/utils/format.hpp"
#include "hut/utils/profiling.hpp"

#include "hut/buffer.hpp"
#include "hut/display.hpp"
#include "hut/image.hpp"

using namespace hut;

std::shared_ptr<image> image::load_raw(display &_display, std::span<const u8> _data, uint _data_row_pitch, const image_params &_params) {
  HUT_PROFILE_SCOPE(PIMAGE, "image::load_raw");
  assert(_params.levels_ == 1);
  assert(_params.layers_ == 1);

  auto dst = std::make_shared<image>(_display, _params);
  dst->update({u16vec4{0, 0, _params.size_}}, _data, _data_row_pitch);

  return dst;
}

image::~image() {
  HUT_PVK(vkDestroyImageView, display_.device(), view_, nullptr);
  HUT_PVK(vkDestroyImage, display_.device(), image_, nullptr);
  HUT_PVK(vkFreeMemory, display_.device(), memory_, nullptr);
}

image::image(display &_display, const image_params &_params)
    : display_(_display)
    , params_(_params) {
  mem_reqs_ = create(_display, _params, &image_, &memory_);

  VkImageViewCreateInfo viewInfo           = {};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image_;
  viewInfo.viewType                        = (params_.flags_ & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = _params.format_;
  viewInfo.subresourceRange.aspectMask     = _params.aspect_;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = _params.levels_;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = _params.layers_;

  if (vkCreateImageView(_display.device(), &viewInfo, nullptr, &view_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }

  if ((_params.usage_ & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
    return;

  VkImageSubresourceRange range;
  range.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
  range.baseMipLevel             = 0;
  range.levelCount               = _params.levels_;
  range.baseArrayLayer           = 0;
  range.layerCount               = _params.layers_;
  display::image_transition pre  = {};
  pre.destination                = image_;
  pre.oldLayout                  = VK_IMAGE_LAYOUT_PREINITIALIZED;
  pre.newLayout                  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  display::image_transition post = {};
  post.destination               = image_;
  post.oldLayout                 = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  post.newLayout                 = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  display::image_clear clear     = {};
  clear.destination              = image_;
  clear.color                    = {};

  std::lock_guard lk(display_.staging_mutex_);
  display_.staging_jobs_++;
  display_.preflush([this, pre, post, clear, range]() {
    display_.stage_transition(pre, range);
    if (params_.tiling_ & VK_IMAGE_TILING_LINEAR)  // FIXME
      display_.stage_clear(clear, range);
    display_.stage_transition(post, range);
    display_.staging_jobs_--;
  });
}

VkMemoryRequirements image::create(display &_display, const image_params &_params,
                                   VkImage *_image, VkDeviceMemory *_imageMemory) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType         = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width      = _params.size_.x;
  imageInfo.extent.height     = _params.size_.y;
  imageInfo.extent.depth      = 1;
  imageInfo.mipLevels         = _params.levels_;
  imageInfo.arrayLayers       = _params.layers_;
  imageInfo.format            = _params.format_;
  imageInfo.tiling            = _params.tiling_;
  imageInfo.initialLayout     = VK_IMAGE_LAYOUT_PREINITIALIZED;
  imageInfo.usage             = _params.usage_;
  imageInfo.samples           = _params.samples_;
  imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.flags             = _params.flags_;

  if (vkCreateImage(_display.device(), &imageInfo, nullptr, _image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memReq;
  HUT_PVK(vkGetImageMemoryRequirements, _display.device(), *_image, &memReq);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memReq.size;
  auto memtype                   = _display.find_memory_type(memReq.memoryTypeBits, _params.properties_);
  allocInfo.memoryTypeIndex      = memtype.first;

  if (vkAllocateMemory(_display.device(), &allocInfo, nullptr, _imageMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  HUT_PVK(vkBindImageMemory, _display.device(), *_image, *_imageMemory, 0);

  return memReq;
}

void image::update(subresource _subres, std::span<const u8> _data, uint _data_row_pitch) {
  auto updto = update(_subres);
  auto size  = bbox_size(_subres.coords_);
  assert(_data.size_bytes() == (_data_row_pitch * size.y));
  auto mip_size = params_.size_ >> _subres.level_;
  assert(mip_size.x >= size.x && mip_size.y >= size.y);

  if (_data_row_pitch == updto.staging_row_pitch() || params_.tiling_ == VK_IMAGE_TILING_OPTIMAL) {
    memcpy(updto.data(), _data.data(), _data.size_bytes());
  } else {
    uint row_bit_size = size.x * bpp();
    assert(row_bit_size >= 8);
    uint row_byte_size = row_bit_size / 8;
    for (uint y = 0; y < size.y; y++) {
      auto *dst_row = updto.data() + y * updto.staging_row_pitch();
      auto *src_row = _data.data() + y * _data_row_pitch;
      memcpy(dst_row, src_row, row_byte_size);
    }
  }
}

image::updator image::update(subresource _subres) {
  auto size         = bbox_size(_subres.coords_);
  uint buffer_align = display_.properties().limits.optimalBufferCopyRowPitchAlignment;
  uint offset_align = std::max(VkDeviceSize(4), display_.properties().limits.optimalBufferCopyOffsetAlignment);
  uint row_bit_size = size.x * bpp();
  assert(row_bit_size >= 8);
  uint row_byte_size     = row_bit_size / 8;
  uint staging_row_pitch = align(row_byte_size, buffer_align);
  auto byte_size         = size.y * staging_row_pitch;

  std::lock_guard lk(display_.staging_mutex_);
  auto            staging_alloc = display_.staging_->allocate_raw(byte_size, offset_align);
  auto            span          = std::span<u8>{staging_alloc->existing_mapping(), byte_size};
  return {*this, std::move(staging_alloc), span, staging_row_pitch, _subres};
}

void image::finalize_update(image::updator &_update) {
  auto size   = bbox_size(_update.subres_.coords_);
  auto origin = bbox_origin(_update.subres_.coords_);

  VkImageSubresourceLayers subresLayers = {};
  subresLayers.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
  subresLayers.mipLevel                 = _update.subres_.level_;
  subresLayers.baseArrayLayer           = _update.subres_.layer_;
  subresLayers.layerCount               = 1;

  VkImageSubresourceRange subresRange = {};
  subresRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
  subresRange.baseMipLevel            = _update.subres_.level_;
  subresRange.levelCount              = 1;
  subresRange.baseArrayLayer          = _update.subres_.layer_;
  subresRange.layerCount              = 1;

  display::image_transition pre   = {};
  pre.destination                 = image_;
  pre.oldLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  pre.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  display::image_transition post  = {};
  post.destination                = image_;
  post.oldLayout                  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  post.newLayout                  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  display::buffer2image_copy copy = {};
  copy.imageExtent                = {(uint)size.x, (uint)size.y, 1};
  copy.imageOffset                = {origin.x, origin.y, 0};
  copy.bufferRowLength            = params_.tiling_ == VK_IMAGE_TILING_OPTIMAL ? 0 : ((_update.staging_row_pitch_ * 8) / bpp());
  copy.bufferImageHeight          = params_.tiling_ == VK_IMAGE_TILING_OPTIMAL ? 0 : size.y;
  copy.bufferOffset               = _update.staging_->offset_bytes();
  copy.imageSubresource           = subresLayers;
  copy.source                     = _update.staging_->underlying_buffer();
  copy.destination                = image_;
  copy.bytesSize                  = _update.size_bytes();

  std::lock_guard lk(display_.staging_mutex_);
  display_.staging_jobs_++;
  display_.preflush([this, pre, post, copy, subresRange]() {
    display_.stage_transition(pre, subresRange);
    display_.stage_copy(copy);
    display_.stage_transition(post, subresRange);
    display_.staging_jobs_--;
  });
  display_.postflush([staging = _update.staging_]() {});
}
