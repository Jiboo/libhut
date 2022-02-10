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

#include "hut/buffer.hpp"

#include "hut/text/font.hpp"

struct hb_buffer_t;

namespace hut::text {

class shaper {
 public:
  using render_mode = font::render_mode;

  shaper() = delete;

  shaper(const shaper &) = delete;
  shaper &operator=(const shaper &) = delete;

  shaper(shaper &&_other) noexcept
      : buffer_(std::exchange(_other.buffer_, nullptr))
      , font_(std::move(_other.font_))
      , rmode_(_other.rmode_) {}
  shaper &operator=(shaper &&_other) noexcept {
    if (&_other != this) {
      buffer_ = std::exchange(_other.buffer_, nullptr);
      font_   = std::move(_other.font_);
    }
    return *this;
  }

  explicit shaper(shared_font _font, render_mode _rmode = shaper::render_mode::NORMAL);
  ~shaper();

  using shape_callback = std::function<void(uint /*index*/, i16vec4 /*quad*/, vec4 /*uv*/, uint /*atlas_page*/)>;
  void shape(const shared_atlas &_atlas, std::u8string_view _text, const shape_callback &_cb);

 private:
  hb_buffer_t *buffer_ = nullptr;
  shared_font  font_;
  render_mode  rmode_;
};

}  // namespace hut::text
