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
#include <codecvt>
#include <functional>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if __has_include("span")
#include <span>
#else
#include "span.hpp"
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/hash.hpp>

namespace hut {

  class buffer_pool;
  class display;
  template<typename TDetails, typename... TExtraBindings> class drawable;
  class font;
  class shaper;
  class image;
  class sampler;
  class window;

  // FIXME JBL: Not in mingw64
#if __has_include("span")
  template<typename TType, size_t TExtent = std::dynamic_extent>
  using span = std::span<TType, TExtent>;
#else
  template<typename TType, size_t TExtent = TCB_SPAN_NAMESPACE_NAME::dynamic_extent>
  using span = TCB_SPAN_NAMESPACE_NAME::span<TType, TExtent>;
#endif

  using shared_buffer = std::shared_ptr<buffer_pool>;
  using shared_font = std::shared_ptr<font>;
  using shared_image = std::shared_ptr<image>;
  using shared_sampler = std::shared_ptr<sampler>;

  using namespace std::chrono_literals;
  using namespace glm;

inline std::string to_utf8(char32_t ch) {
  // NOTE JBL: https://stackoverflow.com/questions/30765256/linker-error-using-vs-2015-rc-cant-find-symbol-related-to-stdcodecvt
#ifdef _MSC_VER
  static std::wstring_convert<std::codecvt_utf8<__int32>, __int32> convert;
#else
  static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
#endif
  try {
    return convert.to_bytes(ch);
  }
  catch (std::range_error& _e) {
    return std::string{};
  }
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

inline vec2 bbox_center(const vec4 &_input) {
  const float width = _input[2] - _input[0];
  const float height = _input[3] - _input[1];
  return vec2{_input[0] + width / 2, _input[1] + height / 2};
}

template<typename T>
inline T align(T _input, T _align) {
  T rest = _input % _align;
  return _input + (rest ? (_align - rest) : 0);
}

template <typename... TArgTypes>
class event {
 public:
  using callback = std::function<bool(TArgTypes...)>;

  void connect(const callback &_callback) {
    cbs_.emplace_back(_callback);
  }

  void once(const callback &_callback) {
    onces_.emplace_back(_callback);
  }

  bool fire(const TArgTypes &... _args) {
    bool handled = false;

    for (auto &cb : cbs_) {
      handled |= cb(_args...);
    }

    for (auto &cb : onces_) {
      handled |= cb(_args...);
    }
    onces_.clear();

    return handled;
  }

  void clear() {
    cbs_.clear();
  }

 protected:
  std::vector<callback> cbs_, onces_;
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

struct dur_t {
    std::chrono::nanoseconds time;

    template <typename TRep, typename TPeriod>
    explicit dur_t(std::chrono::duration<TRep, TPeriod> pTime)
            : time(std::chrono::duration_cast<std::chrono::nanoseconds>(pTime)) {}
};
inline std::ostream &operator<<(std::ostream &pOS, const dur_t &pDur) {
    auto lNano = pDur.time.count();
    if (lNano < 1000)
        return pOS << lNano << "ns";
    else if (lNano < 1'000'000)
        return pOS << lNano / 1000.0 << "µs";
    else if (lNano < 1'000'000'000)
        return pOS << lNano / 1'000'000.0 << "ms";
    return pOS << lNano / 1'000'000'000.0 << "s";
}

inline void hash_combine(std::size_t&) { }

template <typename T, typename... Rest>
inline void hash_combine(std::size_t &_seed, const T &_v, Rest &&..._rest) {
  std::hash<T> hasher;
  _seed ^= hasher(_v) + 0x9e3779b9 + (_seed<<6U) + (_seed>>2U);
  hash_combine(_seed, std::forward<Rest>(_rest)...);
}

template<typename T>
constexpr T bit_width(T _in) {
  // FIXME JBL: Not in mingw64
  return std::numeric_limits<T>::digits - std::countl_zero(_in);
}

template<typename TEnum, TEnum TEnd, typename TUnderlying = uint32_t>
struct flagged {
  static constexpr size_t underlying_bits = sizeof(TUnderlying) * 8;
  static constexpr TUnderlying mask(TEnum _flag) { return 1U << _flag; }
  static_assert(bit_width(mask(TEnd)) <= underlying_bits, "underlying type too small to hold values to end");

  using enum_type = TEnum;
  using underlying_type = TUnderlying;
  static constexpr TUnderlying enum_end = TEnd;

  TUnderlying active_ = 0;

  [[nodiscard]] bool query(TEnum _flag)      const { return active_ & mask(_flag); }
  [[nodiscard]] bool operator[](TEnum _flag) const { return query(_flag); }
  [[nodiscard]] bool operator&(TEnum _flag)  const { return query(_flag); }

  [[nodiscard]] size_t   count()             const { return std::popcount(active_); }
  [[nodiscard]] bool     empty()             const { return active_ == 0; }
  [[nodiscard]] operator bool()              const { return active_ != 0; }

  void reset(TUnderlying _underlying) { active_ = _underlying; }
  flagged &operator=(TUnderlying _underlying) { reset(_underlying); return *this; }

  void flip(TEnum _flag)  { active_ ^= mask(_flag); }
  void set(TEnum _flag)   { active_ |= mask(_flag); }

  flagged operator|(TEnum _flag) const { flagged result{active_}; result.set(_flag); return result; }
  flagged &operator|=(TEnum _flag) { set(_flag); return *this; }

  struct const_iterator {
    TUnderlying shifted_;
    TUnderlying current_;

    const_iterator &operator++() {
      shifted_ = shifted_ >> 1U;
      current_++;
      auto to_skip = std::countr_zero(shifted_);
      shifted_ >>= to_skip;
      current_ = std::min(TUnderlying(underlying_bits), TUnderlying(current_ + to_skip));
      return *this;
    }
    bool operator!=(const const_iterator &_other) const { return current_ != _other.current_; }
    TEnum operator*() const { return static_cast<TEnum>(current_); }
  };

  const_iterator cbegin() const { TUnderlying to_skip = std::countr_zero(active_); return const_iterator{active_ >> to_skip, to_skip}; }
  const_iterator begin() const { return cbegin(); }
  const_iterator cend() const { return const_iterator{0, underlying_bits}; }
  const_iterator end() const { return cend(); }
};

}  // namespace hut
