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

#include "hut/buffer_pool.hpp"
#include "hut/display.hpp"
#include "hut/color.hpp"
#include "hut/utils.hpp"

namespace hut {

class display;

struct image_params {
  u16vec2 size_;
  VkFormat format_;
  u16 levels_ = 1;
  u16 layers_ = 1;
  VkImageTiling tiling_ = VkImageTiling::VK_IMAGE_TILING_LINEAR;
  VkImageUsageFlags usage_ = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  VkImageAspectFlags aspect_ = VK_IMAGE_ASPECT_COLOR_BIT;
  VkMemoryPropertyFlags properties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  VkSampleCountFlagBits samples_ = VK_SAMPLE_COUNT_1_BIT;
  VkImageCreateFlags flags_ = 0;
};

class image {
  friend class display;
  friend class font;
  template<typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments> friend class pipeline;
  friend class render_target;
  friend class window;
  friend class offscreen;

 public:
  struct subresource {
    u16vec4 coords_;
    u16 level_ = 0;
    u16 layer_ = 0;
  };

  class updator {
    friend image;

    image &target_;
    buffer_pool::alloc staging_;
    std::span<u8> data_;
    uint staging_row_pitch_;
    subresource subres_;

    updator(image &_target, buffer_pool::alloc _staging, std::span<u8> _data, uint _staging_row_pitch, subresource _subres)
        : target_(_target), staging_(_staging), data_(_data), staging_row_pitch_(_staging_row_pitch), subres_(_subres) {
    }

   public:
    ~updator() { target_.finalize_update(*this); }

    u8 *data() { return data_.data(); }
    size_t size_bytes() { return data_.size_bytes(); }
    uint staging_row_pitch() { return staging_row_pitch_; }
  };

  static std::shared_ptr<image> load_png(display &, std::span<const u8> _data);
  static std::shared_ptr<image> load_raw(display &, std::span<const u8> _data, uint _data_row_pitch, const image_params &_params);

  image(display &_display, const image_params &_params);
  ~image();

  void update(subresource _subres, std::span<const u8>, uint _srcRowPitch);
  updator prepare_update(subresource _subres);
  updator prepare_update() { return prepare_update({make_bbox_with_origin_size({0,0}, params_.size_)}); }

  [[nodiscard]] u8 bpp() const { return info(params_.format_).bpp(); }
  [[nodiscard]] VkFormat format() const { return params_.format_; }
  [[nodiscard]] u16vec2 size() const { return params_.size_; }
  [[nodiscard]] u16 levels() const { return params_.levels_; }
  [[nodiscard]] u16 layers() const { return params_.layers_; }

 private:
  static VkMemoryRequirements create(display &_display, const image_params &_params,
                             VkImage *_image, VkDeviceMemory *_imageMemory);

  void finalize_update(updator &_update);

  display &display_;
  image_params params_;

  VkImage image_;
  VkDeviceMemory memory_;
  VkMemoryRequirements mem_reqs_;
  VkImageView view_;
};

using shared_image = std::shared_ptr<image>;

class sampler {
  template<typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments> friend class pipeline;

  VkSampler sampler_;
  VkDevice device_;

public:
  explicit sampler(display &_display, bool _hiquality = true, float _maxLod = 0.f);
  sampler(display &_display, VkSamplerCreateInfo *_info);
  ~sampler();
};

using shared_sampler = std::shared_ptr<sampler>;

}  // namespace hut
