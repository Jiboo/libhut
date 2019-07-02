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

#include <glm/glm.hpp>

#include "hut/display.hpp"

namespace hut {

class display;

class image {
  friend class display;
  friend class rgb;
  friend class rgba;
  friend class tex;
  friend class rgb_tex;
  friend class rgba_tex;
  friend class rgba_mask;
  friend class font;

 public:
  static std::shared_ptr<image> load_png(display &, const uint8_t *_data, size_t _size);

  image(display &_display, glm::uvec2 _size, VkFormat _format);
  ~image();

  void update(glm::ivec4 _coords, uint8_t *_data, uint _srcRowPitch);

  uint pixel_size() const { return pixel_size_; }
  glm::uvec2 size() const { return size_; }

 private:
  static VkDeviceSize create(display &_display, uint32_t _width, uint32_t _height, VkFormat _format,
                             VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags _properties,
                             VkImage *_image, VkDeviceMemory *_imageMemory);

  display &display_;
  glm::uvec2 size_;
  VkFormat format_;
  uint pixel_size_;

  VkImage image_;
  VkDeviceMemory memory_;
  VkImageView view_;
};

using shared_image = std::shared_ptr<image>;

class sampler {
  friend class tex;
  friend class rgb_tex;
  friend class rgba_tex;
  friend class rgba_mask;

  VkSampler sampler_;
  VkDevice device_;

public:
  sampler(display &_display, bool _hiquality = true);
  sampler(display &_display, VkSamplerCreateInfo *_info);
  ~sampler();
};

}  // namespace hut
