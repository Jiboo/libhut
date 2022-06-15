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

#include <ratio>

#include "hut/utils/glm.hpp"
#include "hut/utils/math.hpp"
#include "hut/utils/preprocessor.hpp"

namespace hut {

enum unit { PIXEL, POINT, INCH, MILLIMETERS };

#define HUT_FOR_EACH_UNIT(XMACRO, ...)                                                                                 \
  XMACRO(px, PIXEL __VA_OPT__(, ) __VA_ARGS__)                                                                         \
  XMACRO(pt, POINT __VA_OPT__(, ) __VA_ARGS__)                                                                         \
  XMACRO(in, INCH __VA_OPT__(, ) __VA_ARGS__)                                                                          \
  XMACRO(mm, MILLIMETERS __VA_OPT__(, ) __VA_ARGS__)

inline std::ostream &operator<<(std::ostream &_os, unit _u) {
  switch (_u) {
    case PIXEL: return _os << "px";
    case POINT: return _os << "pt";
    case INCH: return _os << "in";
    case MILLIMETERS: return _os << "mm";
    default: return _os << "invalid_unit";
  }
}

template<unit TUnit, typename TUnderlying>
struct length {
 private:
  TUnderlying value_;

 public:
  template<typename TOtherUnderlying>
  using common = length<TUnit, std::common_type_t<TUnderlying, TOtherUnderlying>>;
  template<typename TOtherUnderlying>
  using compat = length<TUnit, TOtherUnderlying>;

  constexpr length()                            = default;
  constexpr length(const length &_o)            = default;
  constexpr length &operator=(const length &_o) = default;
  constexpr ~length()                           = default;

  template<arithmetic TScalar>
  explicit constexpr length(TScalar _s)
      : value_(static_cast<TUnderlying>(_s)) {}
  template<arithmetic TScalar>
  constexpr length &operator=(TScalar _o) {
    value_ = _o;
    return *this;
  }
  template<arithmetic TScalar>
  explicit constexpr operator TScalar() const noexcept {
    return static_cast<TScalar>(value_);
  }

  template<typename TOtherUnderlying>
  constexpr length &operator=(compat<TOtherUnderlying> _o) {
    value_ = static_cast<TOtherUnderlying>(_o);
    return *this;
  }
  template<arithmetic TOtherUnderlying>
  constexpr operator compat<TOtherUnderlying>() const noexcept {
    return compat<TOtherUnderlying>{static_cast<TOtherUnderlying>(value_)};
  }

  template<typename TOtherUnderlying>
  constexpr length &operator+=(compat<TOtherUnderlying> _o) {
    value_ += static_cast<TOtherUnderlying>(_o);
    return *this;
  }
  template<typename TOtherUnderlying>
  constexpr length &operator-=(compat<TOtherUnderlying> _o) {
    value_ -= static_cast<TOtherUnderlying>(_o);
    return *this;
  }
  template<arithmetic TScalar>
  constexpr length &operator*=(TScalar _o) {
    value_ *= _o;
    return *this;
  }
  template<arithmetic TScalar>
  constexpr length &operator/=(TScalar _o) {
    value_ /= _o;
    return *this;
  }
  template<arithmetic TScalar>
  constexpr length &operator>>=(TScalar _o) {
    value_ >>= _o;
    return *this;
  }
  template<arithmetic TScalar>
  constexpr length &operator<<=(TScalar _o) {
    value_ <<= _o;
    return *this;
  }

  template<typename TOtherUnderlying>
  constexpr bool operator==(compat<TOtherUnderlying> _o) const {
    return value_ == static_cast<TOtherUnderlying>(_o);
  }
  template<typename TOtherUnderlying>
  constexpr bool operator!=(compat<TOtherUnderlying> _o) const {
    return value_ != static_cast<TOtherUnderlying>(_o);
  }
  template<typename TOtherUnderlying>
  constexpr bool operator<=(compat<TOtherUnderlying> _o) const {
    return value_ <= static_cast<TOtherUnderlying>(_o);
  }
  template<typename TOtherUnderlying>
  constexpr bool operator>=(compat<TOtherUnderlying> _o) const {
    return value_ >= static_cast<TOtherUnderlying>(_o);
  }
  template<typename TOtherUnderlying>
  constexpr bool operator<(compat<TOtherUnderlying> _o) const {
    return value_ < static_cast<TOtherUnderlying>(_o);
  }
  template<typename TOtherUnderlying>
  constexpr bool operator>(compat<TOtherUnderlying> _o) const {
    return value_ > static_cast<TOtherUnderlying>(_o);
  }

  template<typename TOtherUnderlying>
  constexpr common<TOtherUnderlying> operator+(compat<TOtherUnderlying> _o) const {
    std::common_type_t<TUnderlying, TOtherUnderlying> res = value_ + static_cast<TOtherUnderlying>(_o);
    return common<TOtherUnderlying>{res};
  }
  template<typename TOtherUnderlying>
  constexpr common<TOtherUnderlying> operator-(compat<TOtherUnderlying> _o) const {
    std::common_type_t<TUnderlying, TOtherUnderlying> res = value_ - static_cast<TOtherUnderlying>(_o);
    return common<TOtherUnderlying>{res};
  }
  template<arithmetic TScalar>
  constexpr common<TScalar> operator*(TScalar _o) const {
    return common<TScalar>{value_ * _o};
  }
  template<arithmetic TScalar>
  constexpr common<TScalar> operator/(TScalar _o) const {
    return common<TScalar>{value_ / _o};
  }

  constexpr length operator+() { return length{+value_}; }
  length           operator-() requires(!std::is_unsigned_v<TUnderlying>) {
              // NOTE: -800_px; uses the ull literal, operator- overflows, use -800._px instead for better sign handling
    return length{-value_};
  }

  constexpr length &operator++() {
    value_++;
    return *this;
  }
  constexpr length &operator--() {
    value_++;
    return *this;
  }

  constexpr length operator++(int) {
    length before{*this};
    ++value_;
    return before;
  }
  constexpr length operator--(int) {
    length before{*this};
    --value_;
    return *this;
  }
};

template<unit TUnit, typename TUnderlying>
inline std::ostream &operator<<(std::ostream &_os, length<TUnit, TUnderlying> _n) {
  return _os << static_cast<TUnderlying>(_n) << TUnit;
}

#define HUT_DECLARE_LENGTH_NUMERIC(TYPE, NAME, SUFFIX) using NAME##_##SUFFIX = length_##SUFFIX##_t<TYPE>;

#define HUT_DECLARE_LENGTH(SUFFIX, ENUMERATOR)                                                                         \
  template<typename TUnderlying>                                                                                       \
  using length_##SUFFIX##_t = length<ENUMERATOR, TUnderlying>;                                                         \
  HUT_FOR_EACH_NUMERIC(HUT_DECLARE_LENGTH_NUMERIC, SUFFIX)                                                             \
  constexpr inline length_##SUFFIX##_t<long long unsigned> operator"" _##SUFFIX(long long unsigned _in) {              \
    return length_##SUFFIX##_t<long long unsigned>{_in};                                                               \
  }                                                                                                                    \
  constexpr inline length_##SUFFIX##_t<long double> operator"" _##SUFFIX(long double _in) {                            \
    return length_##SUFFIX##_t<long double>{_in};                                                                      \
  }

HUT_FOR_EACH_UNIT(HUT_DECLARE_LENGTH)

template<unit TUnit, typename TUnderlying, typename TOutput = f32>
constexpr length_px_t<TOutput> scale(length<TUnit, TUnderlying> _length, uint _screen_dpi) {
  constexpr TOutput UNIT_CONVERSION_RATIOS[]{
      // clang-format off
      0,
      1. / 72,
      1,
      5. / 127,  // 127/5 = 25.4, and 1in = 25.4mm
      // clang-format on
  };

  if constexpr (TUnit == PIXEL)
    return _length;

  constexpr double RATIO = UNIT_CONVERSION_RATIOS[to_underlying(TUnit)];
  return length_px_t<TOutput>{static_cast<TOutput>(static_cast<TUnderlying>(_length) * (RATIO * _screen_dpi))};
}

#define HUT_DECLARE_LENGTH_VEC_NUMERIC(TYPE, NAME, SIZE, SUFFIX)                                                       \
  using NAME##vec##SIZE##_##SUFFIX = vec_##SUFFIX##_t<SIZE, TYPE>;
#define HUT_DECLARE_LENGTH_VEC_ALIASES(SIZE, SUFFIX) HUT_FOR_EACH_NUMERIC(HUT_DECLARE_LENGTH_VEC_NUMERIC, SIZE, SUFFIX)

#define HUT_DECLARE_LENGTH_VEC(SUFFIX, ENUMERATOR)                                                                     \
  template<length_t TSize, typename TUnderlying>                                                                       \
  using vec_##SUFFIX##_t = vec<TSize, length_##SUFFIX##_t<TUnderlying>>;                                               \
  HUT_DECLARE_LENGTH_VEC_ALIASES(1, SUFFIX)                                                                            \
  HUT_DECLARE_LENGTH_VEC_ALIASES(2, SUFFIX)                                                                            \
  HUT_DECLARE_LENGTH_VEC_ALIASES(3, SUFFIX)                                                                            \
  HUT_DECLARE_LENGTH_VEC_ALIASES(4, SUFFIX)

HUT_FOR_EACH_UNIT(HUT_DECLARE_LENGTH_VEC)

HUT_SCALAR_OPERATORS(HUT_PACK(length_t TSize, unit TUnit, typename TUnderlying),
                     HUT_PACK(vec<TSize, length<TUnit, TUnderlying>>),
                     HUT_PACK(vec<TSize, length<TUnit, std::common_type_t<TUnderlying, TScalar>>>))

}  // namespace hut

namespace std {
template<hut::unit TUnit, typename TUnderlying>
struct numeric_limits<hut::length<TUnit, TUnderlying>> : public numeric_limits<TUnderlying> {};
}  // namespace std
