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

#include <cassert>
#include <cstring>

#include <iomanip>
#include <sstream>
#include <string_view>

namespace hut {

using namespace std::literals::string_view_literals;

template<typename T, T... TValues>
constexpr T max() {
  T values[] = {TValues...};
  T result   = values[0];
  for (size_t i = 0; i < sizeof...(TValues); i++)
    result = std::max(result, values[i]);
  return result;
}

template<typename T, T... TValues>
constexpr T sum() {
  T values[] = {TValues...};
  T result   = 0;
  for (size_t i = 0; i < sizeof...(TValues); i++)
    result += values[i];
  return result;
}

template<size_t TSize>
struct fixed_string {
  static constexpr size_t SIZE     = TSize;
  static constexpr size_t STR_SIZE = TSize - 1;
  using data_type                  = char[SIZE];

  data_type data_ = {};

  fixed_string()                         = default;
  fixed_string(const fixed_string &)     = default;
  fixed_string(fixed_string &&) noexcept = default;

  constexpr fixed_string(const char *_in, size_t _byte_size) {
    auto min = std::min(STR_SIZE, _byte_size);
    for (size_t i = 0; i != min; ++i)
      data_[i] = _in[i];
    if (_byte_size > min && TSize > 3) {
      data_[min - 3] = '.';  // NOTE JBL: UTF8 sequence for h ellipsis would be 0xE2 0x80 0xA6, so no gains
      data_[min - 2] = '.';
      data_[min - 1] = '.';
    }
    data_[min] = 0;
  }

  constexpr fixed_string(const char (&_in)[TSize])
      : fixed_string(_in, TSize - 1) {}
};

template<size_t TStringSize>
constexpr fixed_string<TStringSize> replace_braces(const char (&_in)[TStringSize]) {
  fixed_string<TStringSize> result{_in, TStringSize};
  for (size_t i = 0; i != TStringSize; ++i) {
    switch (_in[i]) {
      default: result.data_[i] = _in[i]; break;
      case '{': result.data_[i] = '['; break;
      case '}': result.data_[i] = ']'; break;
    }
  }
  return result;
}

template<size_t TMaxSize>
fixed_string<TMaxSize> make_fixed(std::string_view _view) {
  return fixed_string<TMaxSize>{_view.data(), _view.size()};
}

template<size_t TMaxSize>
fixed_string<TMaxSize> make_fixed(std::u8string_view _view) {
  return fixed_string<TMaxSize>{(char *)_view.data(), _view.size()};
}

template<size_t TMaxSize>
fixed_string<TMaxSize> make_fixed(const char *_in) {
  return _in != nullptr ? fixed_string<TMaxSize>{_in, strlen(_in)} : fixed_string<TMaxSize>{nullptr, 0};
}

template<size_t TSize>
inline std::ostream &operator<<(std::ostream &_os, const fixed_string<TSize> &_in) {
  return _os << _in.data_;
}

template<size_t... TSizes>
struct fixed_string_array {
  static constexpr size_t COUNT     = sizeof...(TSizes);
  static constexpr size_t DATA_SIZE = sum<size_t, TSizes...>();
  using data_type                   = char[DATA_SIZE];
  using offsets_type                = size_t[COUNT];

  data_type    data_    = {};
  offsets_type offsets_ = {};

  fixed_string_array(const fixed_string_array &)     = default;
  fixed_string_array(fixed_string_array &&) noexcept = default;

  constexpr fixed_string_array(const char (&..._in)[TSizes]) { init(0, 0, _in...); }

  template<size_t TFirstSize, size_t... TRestSizes>
  constexpr void init(size_t _index, size_t _byte_offset, const char (&_first)[TFirstSize],
                      const char (&..._rest)[TRestSizes]) {
    offsets_[_index] = _byte_offset;
    for (size_t i = 0; i != TFirstSize; ++i)
      data_[i + _byte_offset] = _first[i];
    if constexpr (sizeof...(TRestSizes) > 0)
      init(_index + 1, _byte_offset + TFirstSize, _rest...);
  }

  constexpr void init(size_t, size_t) {}

  [[nodiscard]] constexpr const char *at(size_t _index) const {
    assert(_index < COUNT);
    return data_ + offsets_[_index];
  }
};

template<typename TInput, typename TCallback>
inline void split(std::basic_string_view<TInput> _input, std::basic_string_view<TInput> _delimiters,
                  const TCallback &_callback) {
  // https://github.com/fenbf/StringViewTests/blob/master/StringViewTest.cpp
  auto *last = _input.data() + _input.size();
  for (auto *first = _input.data(), *second = first; second != last && first != last; first = second + 1) {
    second = std::find_first_of(first, last, std::cbegin(_delimiters), std::cend(_delimiters));
    if (first != second)
      _callback(std::basic_string_view<TInput>{first, static_cast<size_t>(second - first)});
  }
}

inline void hexdump(const void *_ptr, size_t _buflen) {
  // https://stackoverflow.com/questions/29242/off-the-shelf-c-hex-dump-code
  auto  *buf = (unsigned char *)_ptr;
  size_t i, j;
  for (i = 0; i < _buflen; i += 16) {
    printf("%06zx: ", i);
    for (j = 0; j < 16; j++) {
      if (i + j < _buflen)
        printf("%02x ", buf[i + j]);
      else
        printf("   ");
    }
    printf(" ");
    for (j = 0; j < 16; j++) {
      if (i + j < _buflen)
        printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
    }
    printf("\n");
  }
}

inline void escape_json(std::ostream &_os, std::string_view _in) {
  std::stringstream buffer;

  for (char c : _in) {
    if (c == '"' || c == '\\' || ('\x00' <= c && c <= '\x1f')) {
      buffer << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
    } else {
      buffer << c;
    }
  }

  _os << buffer.str();
}

}  // namespace hut
