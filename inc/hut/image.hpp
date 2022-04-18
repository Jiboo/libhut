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

#include "hut/utils/format.hpp"

#include "hut/buffer.hpp"
#include "hut/display.hpp"

namespace hut {

class display;

struct image_params {
  u16vec2_px               size_;
  VkFormat              format_;
  u16                   levels_     = 1;
  u16                   layers_     = 1;
  VkImageTiling         tiling_     = VkImageTiling::VK_IMAGE_TILING_LINEAR;
  VkImageUsageFlags     usage_      = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  VkImageAspectFlags    aspect_     = VK_IMAGE_ASPECT_COLOR_BIT;
  VkMemoryPropertyFlags properties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  VkSampleCountFlagBits samples_    = VK_SAMPLE_COUNT_1_BIT;
  VkImageCreateFlags    flags_      = 0;
};

class image {
  friend class offscreen;

 public:
  struct subresource {
    u16vec4_px coords_;
    u16     level_ = 0;
    u16     layer_ = 0;
  };

  class updator {
    friend class image;

    image              *target_ = nullptr;
    buffer_suballoc<u8> staging_;
    std::span<u8>       data_;
    uint                staging_row_pitch_ = -1;
    subresource         subres_;

    updator(image &_target, buffer_suballoc<u8> &&_staging, std::span<u8> _data, uint _staging_row_pitch,
            subresource _subres)
        : target_(&_target)
        , staging_(std::move(_staging))
        , data_(_data)
        , staging_row_pitch_(_staging_row_pitch)
        , subres_(_subres) {}

   public:
    updator() = delete;

    updator(const updator &) = delete;
    updator &operator=(const updator &) = delete;

    updator(updator &&_other) noexcept
        : target_(std::exchange(_other.target_, nullptr))
        , staging_(std::move(_other.staging_))
        , data_(std::exchange(_other.data_, {}))
        , staging_row_pitch_(std::exchange(_other.staging_row_pitch_, 1))
        , subres_(std::exchange(_other.subres_, {})) {}
    updator &operator=(updator &&_other) noexcept {
      if (&_other != this) {
        target_            = std::exchange(_other.target_, nullptr);
        staging_           = std::move(_other.staging_);
        data_              = std::exchange(_other.data_, {});
        staging_row_pitch_ = std::exchange(_other.staging_row_pitch_, 1);
        subres_            = std::exchange(_other.subres_, {});
      }
      return *this;
    }

    ~updator() {
      if (data_.size_bytes() != 0u)
        target_->finalize_update(*this);
    }

    [[nodiscard]] const u8 *data() const { return data_.data(); }
    [[nodiscard]] u8       *data() { return data_.data(); }
    [[nodiscard]] size_t    size_bytes() const { return data_.size_bytes(); }
    [[nodiscard]] uint      staging_row_pitch() const { return staging_row_pitch_; }
    [[nodiscard]] uint      image_row_pitch() const { return target_->bpp() * (uint)bbox_size(subres_.coords_).x / 8; }
  };

  static std::shared_ptr<image> load_raw(display &_display, std::span<const u8> _data, uint _data_row_pitch,
                                         const image_params &_params);

  image() = delete;

  image(const image &) = delete;
  image &operator=(const image &) = delete;

  image(image &&) noexcept = delete;
  image &operator=(image &&) noexcept = delete;

  image(display &_display, const image_params &_params);
  ~image();

  void    update(subresource _subres, std::span<const u8> _data, uint _src_row_pitch);
  updator update(subresource _subres);
  updator update() { return update({make_bbox_with_origin_size({0, 0}, params_.size_)}); }

  [[nodiscard]] u8       bpp() const { return format_info::from(params_.format_).bpp(); }
  [[nodiscard]] VkFormat format() const { return params_.format_; }
  [[nodiscard]] u16vec2_px  size() const { return params_.size_; }
  [[nodiscard]] u16      levels() const { return params_.levels_; }
  [[nodiscard]] u16      layers() const { return params_.layers_; }

  [[nodiscard]] VkImageView view() const { return view_; }

 private:
  static VkMemoryRequirements create(display &_display, const image_params &_params, VkImage *_image,
                                     VkDeviceMemory *_image_memory);

  void finalize_update(updator &_update);

  display     *display_;
  image_params params_;

  VkImage              image_  = VK_NULL_HANDLE;
  VkDeviceMemory       memory_ = VK_NULL_HANDLE;
  VkMemoryRequirements mem_reqs_{};
  VkImageView          view_ = VK_NULL_HANDLE;
};

}  // namespace hut
