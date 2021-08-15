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

#include <chrono>
#include <functional>
#include <locale>
#include <memory>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.h>

namespace hut {

using namespace std::chrono_literals;
using namespace glm;

class buffer_pool;
class display;
template<typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments> class pipeline;
class font;
template<typename TIndexType, typename TVertexType, typename TUpdater> class shaper;
class image;
class sampler;
class window;

using shared_buffer = std::shared_ptr<buffer_pool>;
using shared_font = std::shared_ptr<font>;
using shared_image = std::shared_ptr<image>;
using shared_sampler = std::shared_ptr<sampler>;

inline size_t utf8codepointsize(char32_t _c) {
  // https://github.com/sheredom/utf8.h/blob/master/utf8.h
  if (0 == (0xffffff80u & _c)) {
    return 1;
  } else if (0 == (0xfffff800u & _c)) {
    return 2;
  } else if (0 == (0xffff0000u & _c)) {
    return 3;
  } else { // if (0 == ((int)0xffe00000 & chr)) {
    return 4;
  }
}

inline char8_t *utf8catcodepoint(char8_t *_s, char32_t _c, size_t _n) {
  // https://github.com/sheredom/utf8.h/blob/master/utf8.h

  constexpr char8_t mask_extra = 0x80;
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
  } else { // if (0 == ((int)0xffe00000 & chr)) {
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
  auto size = utf8codepointsize(_src);
  result.resize(size);
  utf8catcodepoint(result.data(), _src, size);
  return result;
}

inline char32_t to_utf32(char16_t c) {
  //https://unicode.org/faq/utf_bom.html#utf16-4
  typedef uint16_t UTF16;
  typedef uint32_t UTF32;
  const UTF32 LEAD_OFFSET = 0xD800 - (0x10000U >> 10U);
  const UTF32 SURROGATE_OFFSET = 0x10000 - (0xD800U << 10U) - 0xDC00;
  const UTF16 uc = c;
  const UTF16 lead = LEAD_OFFSET + (uc >> 10U);
  const UTF16 trail = 0xDC00 + (uc & 0x3FFU);
  const UTF32 codepoint = (lead << 10U) + trail + SURROGATE_OFFSET;
  return codepoint;
}

template<typename T, qualifier Q = defaultp>
inline vec<2, T, Q> bbox_origin(const vec<4, T, Q> &_input) {
  return vec<2, T, Q>{_input[0], _input[1]};
}

template<typename T, qualifier Q = defaultp>
inline vec<2, T, Q> bbox_size(const vec<4, T, Q> &_input) {
  return vec<2, T, Q>{_input[2] - _input[0], _input[3] - _input[1]};
}

template<typename T, qualifier Q = defaultp>
inline vec<2, T, Q> bbox_center(const vec<4, T, Q> &_input) {
  const auto size = bbox_size(_input);
  return vec<2, T, Q>{_input[0] + size.x / 2, _input[1] + size.y / 2};
}

template<typename TBox, typename TPoint, qualifier Q = defaultp>
bool bbox_contains(const vec<4, TBox, Q> &_box, const vec<2, TPoint, Q> &_point) {
  return _point.x >= _box.x && _point.x <= _box.z && _point.y >= _box.y && _point.y <= _box.w;
}

template<typename T, qualifier Q = defaultp>
inline vec<4, T, Q> make_bbox_with_origin_size(const vec<2, T, Q> &_offset, const vec<2, T, Q> &_size) {
  return vec<4, T, Q>{_offset.x, _offset.y, _offset.x + _size.x, _offset.y + _size.y};
}

inline mat4 make_transform_mat4(vec2 _translate, vec3 _scale) {
  mat4 m(1);
  m = translate(m, vec3(_translate, 0));
  m = scale(m, _scale);
  return m;
}

inline mat4 make_transform_mat4(vec2 _translate, float _rot_angle, vec2 _rot_center, vec3 _scale) {
  mat4 m(1);
  m = translate(m, vec3(_translate, 0));
  m = translate(m, vec3(_rot_center, 0));
  m = rotate(m, _rot_angle, {0, 0, 1});
  m = translate(m, vec3(-_rot_center, 0));
  m = scale(m, _scale);
  return m;
}

template<typename T>
inline T align(T _input, unsigned _align) {
  if (_align == 0)
    return _input;

  T rest = _input % _align;
  return _input + (rest ? (_align - rest) : 0);
}

template<typename T, size_t TLeftSize, size_t TRightSize>
constexpr std::array<T, TLeftSize + TRightSize> combine(std::array<T, TLeftSize> _left, std::array<T, TRightSize> _right) {
  std::array<T, TLeftSize + TRightSize> result;
  for (size_t i = 0; i < TLeftSize; i++) {
    result[i] = _left[i];
  }
  for (size_t i = 0; i < TRightSize; i++) {
    result[TLeftSize + i] = _right[i];
  }
  return result;
}

template <typename... TArgTypes>
class event {
 public:
  using callback = std::function<bool(TArgTypes...)>;

  void connect(const callback &_callback) {
    cbs_.emplace_back(_callback);
  }

  bool fire(const TArgTypes &... _args) {
    bool handled = false;

    for (auto &cb : cbs_) {
      handled |= cb(_args...);
      if (handled)
        break;
    }

    return handled;
  }

  void clear() {
    cbs_.clear();
  }

 protected:
  std::vector<callback> cbs_;
};

class sstream {
 private:
  std::ostringstream stream_;

 public:
  template <typename T>
  explicit sstream(const T &_base) {
    stream_ << _base;
  }

  template <typename T>
  sstream &operator<<(const T &_rhs) {
    stream_ << _rhs;
    return *this;
  }

  std::string str() const {
    return stream_.str();
  }

  operator std::string() const {
    return str();
  }
};

inline void hash_combine(std::size_t&) { }

template <typename T, typename... Rest>
inline void hash_combine(std::size_t &_seed, const T &_v, Rest &&..._rest) {
  std::hash<T> hasher;
  _seed ^= hasher(_v) + 0x9e3779b9 + (_seed<<6U) + (_seed>>2U);
  hash_combine(_seed, std::forward<Rest>(_rest)...);
}

template<typename TEnum, TEnum TEnd, typename TUnderlying = uint32_t>
struct flagged {
  static constexpr size_t underlying_bits = sizeof(TUnderlying) * 8;
  static constexpr TUnderlying mask(TEnum _flag) { return 1U << _flag; }
  static_assert(std::bit_width(mask(TEnd)) <= underlying_bits, "underlying type too small to hold values to end");

  using enum_type = TEnum;
  using underlying_type = TUnderlying;
  static constexpr TUnderlying enum_end = TEnd;

  TUnderlying active_ = 0;

  constexpr flagged() = default;
  constexpr explicit flagged(TUnderlying _init) : active_(_init) {}
  constexpr explicit flagged(TEnum _init) { set(_init); }
  constexpr flagged(std::initializer_list<TEnum> _inits) { for (auto flag : _inits) set(flag); }

  constexpr flagged& operator=(const flagged &_other) { active_ = _other.active_; return *this; }
  constexpr flagged& operator=(TUnderlying _init) { active_ = _init; return *this; }
  constexpr flagged& operator=(TEnum _init) { set(_init); return *this; }

  [[nodiscard]] constexpr bool query(TEnum _flag)      const { return active_ & mask(_flag); }
  [[nodiscard]] constexpr bool operator[](TEnum _flag) const { return query(_flag); }
  [[nodiscard]] constexpr bool operator&(TEnum _flag)  const { return query(_flag); }

  [[nodiscard]] constexpr size_t   count()             const { return (size_t)std::popcount(active_); }
  [[nodiscard]] constexpr bool     empty()             const { return active_ == 0; }
  [[nodiscard]] constexpr explicit operator bool()              const { return active_ != 0; }

  constexpr void reset(TEnum _flag) { active_ &= ~mask(_flag); }
  constexpr void flip(TEnum _flag)  { active_ ^= mask(_flag); }
  constexpr void set(TEnum _flag)   { active_ |= mask(_flag); }

  constexpr flagged operator|(TEnum _flag) const { return flagged{active_ | mask(_flag)}; }
  constexpr flagged &operator|=(TEnum _flag) { active_ |= mask(_flag); return *this; }
  constexpr flagged &operator&=(TEnum _flag) { active_ &= mask(_flag); return *this; }

  constexpr flagged operator|(const flagged &_other) const { return flagged{active_ | _other.active_}; }
  constexpr flagged &operator|=(const flagged &_other) { active_ |= _other.active_; return *this; }
  constexpr flagged operator&(const flagged &_other) const { return flagged{active_ & _other.active_}; }
  constexpr flagged &operator&=(const flagged &_other) { active_ &= _other.active_; return *this; }

  constexpr bool operator==(TEnum _flag) const { return active_ == mask(_flag); }
  constexpr bool operator==(const flagged &_other) const { return active_ == _other.active_; }

  struct const_iterator {
    TUnderlying shifted_;
    TUnderlying current_;

    constexpr const_iterator &operator++() {
      shifted_ = shifted_ >> 1U;
      current_++;
      auto to_skip = std::countr_zero(shifted_);
      shifted_ >>= to_skip;
      current_ = std::min(TUnderlying(underlying_bits), TUnderlying(current_ + to_skip));
      return *this;
    }
    constexpr bool operator!=(const const_iterator &_other) const { return current_ != _other.current_; }
    constexpr TEnum operator*() const { return static_cast<TEnum>(current_); }
  };

  constexpr const_iterator cbegin() const { TUnderlying to_skip = std::countr_zero(active_); return const_iterator{active_ >> to_skip, to_skip}; }
  constexpr const_iterator begin() const { return cbegin(); }
  constexpr const_iterator cend() const { return const_iterator{0, underlying_bits}; }
  constexpr const_iterator end() const { return cend(); }
};

template<typename TEnum, TEnum TEnd, typename TUnderlying = uint32_t>
inline std::ostream &operator<<(std::ostream &_os, flagged<TEnum, TEnd, TUnderlying> _flags) {
  _os << "(";
  for (auto it = _flags.cbegin(); it != _flags.end(); ++it) {
    _os << *it;
    if (auto next = it; ++next != _flags.end())
      _os << ",";
  }
  return _os << ")";
}

inline glm::i16vec2 offset2_16(VkOffset2D _in) { return glm::i16vec2{_in.x, _in.y}; }
inline glm::i16vec3 offset3_16(VkOffset3D _in) { return glm::i16vec3{_in.x, _in.y, _in.z}; }
inline glm::i16vec2 extent2_16(VkExtent2D _in) { return glm::u16vec2{_in.width, _in.height}; }
inline glm::i16vec3 extent3_16(VkExtent3D _in) { return glm::u16vec3{_in.width, _in.height, _in.depth}; }
inline glm::vec4 color3_32(VkClearColorValue _in) { return glm::vec4{_in.float32[0], _in.float32[1], _in.float32[2], _in.float32[3]}; }

inline void hexdump(const void *ptr, size_t buflen) {
  // https://stackoverflow.com/questions/29242/off-the-shelf-c-hex-dump-code
  auto *buf = (unsigned char*)ptr;
  size_t i, j;
  for (i=0; i<buflen; i+=16) {
    printf("%06zx: ", i);
    for (j=0; j<16; j++)
      if (i+j < buflen)
        printf("%02x ", buf[i+j]);
      else
        printf("   ");
    printf(" ");
    for (j=0; j<16; j++)
      if (i+j < buflen)
        printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
    printf("\n");
  }
}

template <typename TRep, typename TPeriod>
inline std::ostream &operator<<(std::ostream &_os, const std::chrono::duration<TRep, TPeriod> &_dur) {
  auto nano = std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(_dur).count();
  if (nano < 1000)
    return _os << nano << "ns";
  else if (nano < 1'000'000)
    return _os << nano / 1000.0 << "µs";
  else if (nano < 1'000'000'000)
    return _os << nano / 1'000'000.0 << "ms";
  return _os << nano / 1'000'000'000.0 << "s";
}

template <typename TClock, typename TDur>
inline std::ostream &operator<<(std::ostream &_os, const std::chrono::time_point<TClock, TDur> &_tp) {
  auto elapsed = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(_tp.time_since_epoch());
  return _os << elapsed.count();
}

}  // namespace hut
