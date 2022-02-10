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

namespace hut {

class subimage {
 private:
  atlas  *atlas_ = nullptr;
  uint    page_{};
  u16vec4 bounds_{};

 public:
  subimage() = default;

  subimage(const subimage &) = delete;

  subimage(subimage &&_other) noexcept
      : atlas_(std::exchange(_other.atlas_, nullptr))
      , page_(_other.page_)
      , bounds_(_other.bounds_) {}
  subimage &operator=(subimage &&_other) noexcept {
    if (&_other != this) {
      atlas_  = std::exchange(_other.atlas_, nullptr);
      page_   = _other.page_;
      bounds_ = _other.bounds_;
    }
    return *this;
  }

  subimage(atlas *_atlas, uint _page, uvec4 _bounds)
      : atlas_(_atlas)
      , page_(_page)
      , bounds_(_bounds) {}
  ~subimage() {
    if (atlas_ != nullptr)
      release();
  }

  explicit operator bool() const { return valid(); }

  void               release();
  [[nodiscard]] bool from(const atlas *_pool) const { return atlas_ == _pool; }

  [[nodiscard]] bool    valid() const { return atlas_ != nullptr; }
  [[nodiscard]] uint    page() const { return page_; }
  [[nodiscard]] u16vec4 bounds() const { return bounds_; }
  [[nodiscard]] vec4    texcoords() const;

  image::updator update(u16vec4 _bounds);
  image::updator update() { return update(bounds_); }
};

}  // namespace hut
