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

#include <png.h>

#include "hut/display.hpp"
#include "hut/image.hpp"
#include "hut/color.hpp"
#include "hut/buffer_pool.hpp"

using namespace hut;

struct png_ctx {
  const uint8_t *data_;
  size_t size_;
};

void png_mem_read(png_structp _png_ptr, png_bytep _target, png_size_t _size) {
  auto *a = (png_ctx *)png_get_io_ptr(_png_ptr);

  memcpy(_target, a->data_, _size);
  a->data_ += _size;
}

std::shared_ptr<image> image::load_png(display &_display, span<const uint8_t> _data) {
  HUT_PROFILE_SCOPE(PIMAGE, "image::load_png");
  if (png_sig_cmp((png_bytep)_data.data(), 0, 8))
    throw std::runtime_error("load_png: invalid data, can't validate PNG signature");

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

  if (!png_ptr)
    throw std::runtime_error("load_png: png_create_read_struct failed");

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    throw std::runtime_error("load_png: png_create_info_struct failed");
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    throw std::runtime_error("load_png: Error during init_io");
  }

  png_ctx ctx;
  ctx.data_ = _data.data();
  ctx.size_ = _data.size_bytes();
  png_set_read_fn(png_ptr, (png_voidp)&ctx, png_mem_read);

  png_read_info(png_ptr, info_ptr);

  png_uint_32 channels = png_get_channels(png_ptr, info_ptr);
  png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);
  png_uint_32 bitdepth = png_get_bit_depth(png_ptr, info_ptr);

  if (bitdepth < 8) {
    png_set_expand_gray_1_2_4_to_8(png_ptr);
    bitdepth = 8;
  }
  else if (bitdepth > 8) {
    png_set_strip_16(png_ptr);
    bitdepth = 8;
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
    channels = 3;
  }

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png_ptr);
    channels += 1;
  }

  if (channels == 3) {
    png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
    channels += 1;
  }

  assert(bitdepth == 8);
  assert(channels == 4 || channels == 2 || channels == 1);  // RGBA/LA/L

  VkFormat format;
  switch (channels) {
    case 1:
      format = VK_FORMAT_R8_UNORM;
      break;
    case 2:
      format = VK_FORMAT_R8G8_UNORM;
      break;
    case 4:
      format = VK_FORMAT_R8G8B8A8_UNORM;
      break;
    default:
      throw std::runtime_error("unexpected png channel count");
  }

  auto width = png_get_image_width(png_ptr, info_ptr);
  auto height = png_get_image_height(png_ptr, info_ptr);

  png_read_update_info(png_ptr, info_ptr);
  uint offsetAlignment = std::max(VkDeviceSize(4), _display.device_props_.limits.optimalBufferCopyOffsetAlignment);
  uint rowAlignment = _display.device_props_.limits.optimalBufferCopyRowPitchAlignment;
  uint stride = align(width, rowAlignment);
  uint row_byte_size = stride * channels;
  uint byte_size = row_byte_size * height;

  VkImageSubresourceLayers subResource = {};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subResource.baseArrayLayer = 0;
  subResource.mipLevel = 0;
  subResource.layerCount = 1;

  image_params params;
  params.size_ = {width, height};
  params.format_ = format;
  auto dst = std::make_shared<image>(_display, params);
  shared_ref<u8> staging;
  {
    std::lock_guard lk(_display.staging_mutex_);
    staging = _display.staging_->allocate<u8>(byte_size, offsetAlignment);
  }

  auto dataBytes = staging->buff().permanent_map_ + staging->offset_bytes();
  png_bytep rows[height];
  for (uint y = 0; y < height; y++) {
    rows[y] = dataBytes + y * row_byte_size;
  }
  png_read_image(png_ptr, rows);

  display::buffer2image_copy copy = {};
  copy.imageExtent = {width, height, 1};
  copy.imageOffset = {0, 0, 0};
  copy.bufferRowLength = stride;
  copy.bufferImageHeight = height;
  copy.bufferOffset = staging->offset_bytes();
  copy.imageSubresource = subResource;
  copy.source = staging->vkBuffer();
  copy.destination = dst->image_;
  copy.pixelSize = channels;

  display::image_transition pre = {};
  pre.destination = dst->image_;
  pre.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  pre.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  display::image_transition post = {};
  post.destination = dst->image_;
  post.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  post.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  std::lock_guard lk(_display.staging_mutex_);
  _display.staging_jobs_++;
  _display.preflush([&_display, pre, post, copy, staging = std::move(staging)] {
    _display.stage_transition(pre);
    _display.stage_copy(copy);
    _display.stage_transition(post);
    _display.staging_jobs_--;
  });
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

  return dst;
}

std::shared_ptr<image> image::load_raw(display &_display, span<const uint8_t> _data, const image_params &_params) {
  HUT_PROFILE_SCOPE(PIMAGE, "image::load_raw");
  auto dst = std::make_shared<image>(_display, _params);

  uint offsetAlignment = std::max(VkDeviceSize(4), _display.device_props_.limits.optimalBufferCopyOffsetAlignment);
  {
    shared_ref<u8> staging;
    {
      std::lock_guard lk(_display.staging_mutex_);
      staging = _display.staging_->allocate<u8>(_data.size_bytes(), offsetAlignment);
    }

    auto dataBytes = staging->buff().permanent_map_ + staging->offset_bytes();
    memcpy(dataBytes, _data.data(), _data.size_bytes());

    VkImageSubresourceLayers subResource = {};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResource.baseArrayLayer = 0;
    subResource.layerCount = 1;
    subResource.mipLevel = 0;

    display::buffer2image_copy copy = {};
    copy.imageExtent = {_params.size_.x, _params.size_.y, 1};
    copy.imageOffset = {0, 0, 0};
    copy.bufferRowLength = _params.size_.x;
    copy.bufferImageHeight = _params.size_.y;
    copy.bufferOffset = staging->offset_bytes();
    copy.imageSubresource = subResource;
    copy.source = staging->vkBuffer();
    copy.destination = dst->image_;
    copy.pixelSize = dst->pixel_size_;

    display::image_transition pre = {};
    pre.destination = dst->image_;
    pre.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    pre.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    display::image_transition post = {};
    post.destination = dst->image_;
    post.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    post.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::lock_guard lk(_display.staging_mutex_);
    _display.staging_jobs_++;
    _display.preflush([&_display, pre, post, copy, staging = std::move(staging)] {
      _display.stage_transition(pre);
      _display.stage_copy(copy);
      _display.stage_transition(post);
      _display.staging_jobs_--;
    });
  }
  return dst;
}

image::~image() {
  HUT_PVK(vkDestroyImageView, display_.device_, view_, nullptr);
  HUT_PVK(vkDestroyImage, display_.device_, image_, nullptr);
  HUT_PVK(vkFreeMemory, display_.device_, memory_, nullptr);
}

image::image(display &_display, const image_params &_params)
    : display_(_display), params_(_params) {
  HUT_PROFILE_SCOPE(PIMAGE, "image::image");
  pixel_size_ = format_size(_params.format_);
  mem_reqs_ = create(_display, _params, &image_, &memory_);

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image_;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = _params.format_;
  viewInfo.subresourceRange.aspectMask = _params.aspect_;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(_display.device_, &viewInfo, nullptr, &view_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }

  if ((_params.usage_ & (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)) == 0)
    return;

  display::image_transition pre = {};
  pre.destination = image_;
  pre.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
  pre.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  display::image_transition post = {};
  post.destination = image_;
  post.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  post.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  display::image_clear clear = {};
  clear.destination = image_;
  clear.color = VkClearColorValue{};

  std::lock_guard lk(display_.staging_mutex_);
  display_.staging_jobs_++;
  display_.preflush([this, pre, post, clear]() {
    display_.stage_transition(pre);
    if (this->pixel_size_ != 0)
      display_.stage_clear(clear);
    display_.stage_transition(post);
    display_.staging_jobs_--;
  });
}

VkMemoryRequirements image::create(display &_display, const image_params &_params,
                           VkImage *_image, VkDeviceMemory *_imageMemory) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = _params.size_.x;
  imageInfo.extent.height = _params.size_.y;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = _params.format_;
  imageInfo.tiling = _params.tiling_;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
  imageInfo.usage = _params.usage_;
  imageInfo.samples = _params.samples_;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(_display.device_, &imageInfo, nullptr, _image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memReq;
  HUT_PVK(vkGetImageMemoryRequirements, _display.device_, *_image, &memReq);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memReq.size;
  auto memtype = _display.find_memory_type(memReq.memoryTypeBits, _params.properties_);
  allocInfo.memoryTypeIndex = memtype.first;

  if (vkAllocateMemory(_display.device_, &allocInfo, nullptr, _imageMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  HUT_PVK(vkBindImageMemory, _display.device_, *_image, *_imageMemory, 0);

  return memReq;
}

void image::update(ivec4 coords, uint8_t *_data, uint _src_row_pitch) {
  uint width = coords[2] - coords[0];
  uint height = coords[3] - coords[1];
  assert (width > 0 && height > 0);
  uint buffer_align = display_.device_props_.limits.optimalBufferCopyRowPitchAlignment;
  uint offset_align = std::max(VkDeviceSize(4), display_.device_props_.limits.optimalBufferCopyOffsetAlignment);
  uint row_byte_size = width * pixel_size_;
  uint buffer_row_pitch = align(row_byte_size, buffer_align);
  auto byte_size = height * buffer_row_pitch;

  shared_ref<u8> staging;
  {
    std::lock_guard lk(display_.staging_mutex_);
    staging = display_.staging_->allocate<u8>(byte_size, offset_align);
  }
  auto dataBytes = staging->buff().permanent_map_ + staging->offset_bytes();
  for (uint i = 0; i < height; i++) {
    uint8_t *src = _data + _src_row_pitch * i;
    uint8_t *dst = dataBytes + buffer_row_pitch * i;
    memcpy(dst, src, row_byte_size);
  }

  VkImageSubresourceLayers subResource = {};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subResource.baseArrayLayer = 0;
  subResource.mipLevel = 0;
  subResource.layerCount = 1;

  display::image_transition pre = {};
  pre.destination = image_;
  pre.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  pre.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  display::image_transition post = {};
  post.destination = image_;
  post.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  post.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  display::buffer2image_copy copy = {};
  copy.imageExtent = {(uint)width, (uint)height, 1};
  copy.imageOffset = {coords[0], coords[1], 0};
  copy.bufferRowLength = buffer_row_pitch / pixel_size_;
  copy.bufferImageHeight = height;
  copy.bufferOffset = staging->offset_bytes();
  copy.imageSubresource = subResource;
  copy.source = staging->vkBuffer();
  copy.destination = image_;
  copy.pixelSize = pixel_size_;

  std::lock_guard lk(display_.staging_mutex_);
  display_.staging_jobs_++;
  display_.preflush([this, pre, post, copy, staging = std::move(staging)]() {
    display_.stage_transition(pre);
    display_.stage_copy(copy);
    display_.stage_transition(post);
    display_.staging_jobs_--;
  });
}

sampler::sampler(display &_display, VkSamplerCreateInfo *_info) : device_(_display.device_) {
  if (vkCreateSampler(device_, _info, nullptr, &sampler_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

sampler::sampler(display &_display, bool _hiquality) : device_(_display.device_) {
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = _hiquality ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
  samplerInfo.minFilter = _hiquality ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  bool enable_filtering = _hiquality && _display.device_features_.samplerAnisotropy;
  samplerInfo.anisotropyEnable = enable_filtering ? VK_TRUE : VK_FALSE;
  samplerInfo.maxAnisotropy = enable_filtering ? _display.device_props_.limits.maxSamplerAnisotropy : 0;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
  if (vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

sampler::~sampler() {
  HUT_PVK(vkDeviceWaitIdle, device_);
  HUT_PVK(vkDestroySampler, device_, sampler_, nullptr);
}
