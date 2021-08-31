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

#include <string_view>

namespace hut {

using namespace std::literals::string_view_literals;

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

inline void hexdump(const void *ptr, size_t buflen) {
  // https://stackoverflow.com/questions/29242/off-the-shelf-c-hex-dump-code
  auto * buf = (unsigned char *)ptr;
  size_t i, j;
  for (i = 0; i < buflen; i += 16) {
    printf("%06zx: ", i);
    for (j = 0; j < 16; j++) {
      if (i + j < buflen)
        printf("%02x ", buf[i + j]);
      else
        printf("   ");
    }
    printf(" ");
    for (j = 0; j < 16; j++) {
      if (i + j < buflen)
        printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
    }
    printf("\n");
  }
}

}  // namespace hut
