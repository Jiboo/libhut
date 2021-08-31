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

#include <sstream>
#include <string>
#include <string_view>

namespace hut {

class sstream {
 private:
  std::ostringstream stream_;

 public:
  sstream() noexcept       = default;
  sstream(const sstream &) = delete;
  sstream(sstream &&_base) noexcept
      : stream_(std::move(_base.stream_)) {}

  explicit sstream(const char *_base) { stream_ << _base; }
  explicit sstream(const std::string &_base) { stream_ << _base; }
  explicit sstream(const std::string_view _base) { stream_ << _base; }

  template<typename T>
  sstream &operator<<(const T &_rhs) {
    stream_ << _rhs;
    return *this;
  }

  std::string str() const {
    return stream_.str();
  }

  std::string_view view() const {
    return stream_.view();
  }

  operator std::string() const {
    return str();
  }

  operator std::string_view() const {
    return view();
  }
};

}  // namespace hut
