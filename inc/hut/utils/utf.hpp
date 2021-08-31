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

#include <span>
#include <string>
#include <string_view>

#include "hut/utils/math.hpp"

namespace hut {

inline size_t utf8codepointcount(std::u8string_view s) {
  // https://stackoverflow.com/a/7298458
  size_t     len  = 0;
  const auto last = s.data() + s.size();
  for (auto cur = s.data(); cur != last; cur++)
    if ((*cur & 0xC0) != 0x80) ++len;
  return len;
}

inline size_t utf8codepointsize(char32_t _c) {
  // https://github.com/sheredom/utf8.h/blob/master/utf8.h
  if (0 == (0xffffff80u & _c)) {
    return 1;
  } else if (0 == (0xfffff800u & _c)) {
    return 2;
  } else if (0 == (0xffff0000u & _c)) {
    return 3;
  } else {  // if (0 == ((int)0xffe00000 & chr)) {
    return 4;
  }
}

inline char8_t *utf8catcodepoint(char8_t *_s, char32_t _c, size_t _n) {
  // https://github.com/sheredom/utf8.h/blob/master/utf8.h

  constexpr char8_t mask_extra  = 0x80;
  constexpr char8_t mask_2bytes = 0xc0;
  constexpr char8_t mask_3bytes = 0xe0;
  constexpr char8_t mask_4bytes = 0xf0;

  if (0 == (0xffffff80u & _c)) {
    // 1-byte/7-bit ascii
    // (0b0xxxxxxx)
    if (_n < 1) {
      return _s;
    }
    _s[0] = (char8_t)_c;
    _s += 1;
  } else if (0 == (0xfffff800u & _c)) {
    // 2-byte/11-bit utf8 code point
    // (0b110xxxxx 0b10xxxxxx)
    if (_n < 2) {
      return _s;
    }
    _s[0] = mask_2bytes | (char8_t)(_c >> 6);
    _s[1] = mask_extra | (char8_t)(_c & 0x3f);
    _s += 2;
  } else if (0 == (0xffff0000u & _c)) {
    // 3-byte/16-bit utf8 code point
    // (0b1110xxxx 0b10xxxxxx 0b10xxxxxx)
    if (_n < 3) {
      return _s;
    }
    _s[0] = mask_3bytes | (char8_t)(_c >> 12);
    _s[1] = mask_extra | (char8_t)((_c >> 6) & 0x3f);
    _s[2] = mask_extra | (char8_t)(_c & 0x3f);
    _s += 3;
  } else {  // if (0 == ((int)0xffe00000 & chr)) {
    // 4-byte/21-bit utf8 code point
    // (0b11110xxx 0b10xxxxxx 0b10xxxxxx 0b10xxxxxx)
    if (_n < 4) {
      return _s;
    }
    _s[0] = mask_4bytes | (char8_t)(_c >> 18);
    _s[1] = mask_extra | (char8_t)((_c >> 12) & 0x3f);
    _s[2] = mask_extra | (char8_t)((_c >> 6) & 0x3f);
    _s[3] = mask_extra | (char8_t)(_c & 0x3f);
    _s += 4;
  }

  return _s;
}

inline char8_t *to_utf8(std::span<char8_t> _dst, char32_t _src) {
  return utf8catcodepoint(_dst.data(), _src, _dst.size());
}

inline std::u8string to_utf8(char32_t _src) {
  std::u8string result;
  auto          size = utf8codepointsize(_src);
  result.resize(size);
  utf8catcodepoint(result.data(), _src, size);
  return result;
}

inline char32_t to_utf32(char16_t c) {
  //https://unicode.org/faq/utf_bom.html#utf16-4
  typedef u16 UTF16;
  typedef u32 UTF32;
  const UTF32 LEAD_OFFSET      = 0xD800 - (0x10000U >> 10U);
  const UTF32 SURROGATE_OFFSET = 0x10000 - (0xD800U << 10U) - 0xDC00;
  const UTF16 uc               = c;
  const UTF16 lead             = LEAD_OFFSET + (uc >> 10U);
  const UTF16 trail            = 0xDC00 + (uc & 0x3FFU);
  const UTF32 codepoint        = (lead << 10U) + trail + SURROGATE_OFFSET;
  return codepoint;
}

}  // namespace hut
