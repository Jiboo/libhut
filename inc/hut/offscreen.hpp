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

#include "hut/image.hpp"
#include "hut/render_target.hpp"

namespace hut {
struct offscreen_params {
  enum flag {
    FDEPTH          = render_target_params::flag::FDEPTH,
    FMULTISAMPLING  = render_target_params::flag::FMULTISAMPLING,
    FLAG_LAST_VALUE = FMULTISAMPLING,
  };
  using flags = flagged<flag, flag::FLAG_LAST_VALUE>;

  flags              flags_{};
  image::subresource subres_{};
};

class offscreen : public render_target {
 public:
  offscreen() = delete;

  offscreen(const offscreen &) = delete;
  offscreen &operator=(const offscreen &) = delete;

  offscreen(offscreen &&) noexcept = delete;
  offscreen &operator=(offscreen &&) noexcept = delete;

  explicit offscreen(const shared_image &_target, offscreen_params _init_params = {});
  ~offscreen();

  using draw_callback = std::function<void(VkCommandBuffer)>;
  void draw(const draw_callback &_callback);

  void download(std::span<u8> _dst, uint _data_row_pitch, image::subresource _src = {});

 protected:
  offscreen_params params_;
  shared_image     target_;
  VkCommandBuffer  cb_    = VK_NULL_HANDLE;
  VkFence          fence_ = VK_NULL_HANDLE;

  void flush_cb();
};

}  // namespace hut
