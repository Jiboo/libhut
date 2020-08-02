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

#pragma once

#include <memory>
#include <set>

#include <vulkan/vulkan.h>

#include "hut/utils.hpp"
#include "display.hpp"
#include "buffer_pool.hpp"


namespace hut {

class display;

class image {
  friend class display;
  friend class font;
  template<typename TDetails, typename... TExtraBindings> friend class drawable;
  friend class window;

 public:
  static constexpr VkImageTiling defaultImageTiling = VkImageTiling::VK_IMAGE_TILING_LINEAR;
  static constexpr VkImageUsageFlags defaultImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  static std::shared_ptr<image> load_png(display &, span<const uint8_t> _data);
  static std::shared_ptr<image> load_raw(display &, uvec2 _extent, span<const uint8_t> _data, VkFormat _format,
                                         VkImageTiling _tiling = defaultImageTiling,
                                         VkImageUsageFlags _flags = defaultImageUsageFlags);

  image(display &_display, uvec2 _size, VkFormat _format,
      VkImageTiling _tiling = defaultImageTiling, VkImageUsageFlags _flags = defaultImageUsageFlags,
      VkImageAspectFlags _aspect = VK_IMAGE_ASPECT_COLOR_BIT);
  ~image();

  void update(ivec4 _coords, uint8_t *_data, uint _srcRowPitch);

  [[nodiscard]] uint pixel_size() const { return pixel_size_; }
  [[nodiscard]] uvec2 size() const { return size_; }

 private:
  static VkMemoryRequirements create(display &_display, uint32_t _width, uint32_t _height, VkFormat _format,
                             VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags _properties,
                             VkImage *_image, VkDeviceMemory *_imageMemory);

  display &display_;
  uvec2 size_;
  VkFormat format_;
  uint pixel_size_;

  VkImage image_;
  VkDeviceMemory memory_;
  VkMemoryRequirements mem_reqs_;
  VkImageView view_;
};

using shared_image = std::shared_ptr<image>;

class sampler {
  template<typename TDetails, typename... TExtraBindings> friend class drawable;

  VkSampler sampler_;
  VkDevice device_;

public:
  explicit sampler(display &_display, bool _hiquality = true);
  sampler(display &_display, VkSamplerCreateInfo *_info);
  ~sampler();
};

using shared_sampler = std::shared_ptr<sampler>;

}  // namespace hut
