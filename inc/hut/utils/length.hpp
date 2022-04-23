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

#include "hut/utils/math.hpp"

namespace hut {

enum unit { PIXEL, POINT, INCH, MILLIMETERS };

inline std::ostream &operator<<(std::ostream &_os, unit _u) {
  switch (_u) {
    case PIXEL: return _os << "px";
    case POINT: return _os << "pt";
    case INCH: return _os << "in";
    case MILLIMETERS: return _os << "mm";
    default: return _os << "invalid_unit";
  }
}

template<typename TUnderlying, unit TUnit>
struct length {
 private:
  TUnderlying value_;

 public:
  template<typename TOtherUnderlying>
  using common = length<std::common_type_t<TUnderlying, TOtherUnderlying>, TUnit>;
  template<typename TOtherUnderlying>
  using compat = length<TOtherUnderlying, TUnit>;

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
  length           operator-()
    requires(!std::is_unsigned_v<TUnderlying>)
  {
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

template<typename TUnderlying, unit TUnit>
inline std::ostream &operator<<(std::ostream &_os, length<TUnderlying, TUnit> _n) {
  return _os << static_cast<TUnderlying>(_n) << TUnit;
}

#define HUT_DECLARE_LENGTH(UNIT_SUFFIX, UNIT_ENUMERATOR)                                                               \
  template<typename TUnderlying>                                                                                       \
  using length_##UNIT_SUFFIX = length<TUnderlying, UNIT_ENUMERATOR>;                                                   \
  using u8_##UNIT_SUFFIX     = length_##UNIT_SUFFIX<u8>;                                                               \
  using i8_##UNIT_SUFFIX     = length_##UNIT_SUFFIX<i8>;                                                               \
  using u16_##UNIT_SUFFIX    = length_##UNIT_SUFFIX<u16>;                                                              \
  using i16_##UNIT_SUFFIX    = length_##UNIT_SUFFIX<i16>;                                                              \
  using u32_##UNIT_SUFFIX    = length_##UNIT_SUFFIX<u32>;                                                              \
  using i32_##UNIT_SUFFIX    = length_##UNIT_SUFFIX<i32>;                                                              \
  using f32_##UNIT_SUFFIX    = length_##UNIT_SUFFIX<f32>;                                                              \
  using f64_##UNIT_SUFFIX    = length_##UNIT_SUFFIX<f64>;                                                              \
  constexpr inline length_##UNIT_SUFFIX<long long unsigned> operator"" _##UNIT_SUFFIX(long long unsigned _in) {        \
    return length_##UNIT_SUFFIX<long long unsigned>{_in};                                                              \
  }                                                                                                                    \
  constexpr inline length_##UNIT_SUFFIX<long double> operator"" _##UNIT_SUFFIX(long double _in) {                      \
    return length_##UNIT_SUFFIX<long double>{_in};                                                                     \
  }

HUT_DECLARE_LENGTH(px, PIXEL)
HUT_DECLARE_LENGTH(pt, POINT)
HUT_DECLARE_LENGTH(in, INCH)
HUT_DECLARE_LENGTH(mm, MILLIMETERS)

template<typename TOutput = f32, typename TUnderlying, unit TUnit>
constexpr length_px<TOutput> scale(length<TUnderlying, TUnit> _length, uint _screen_dpi) {
  constexpr TOutput UNIT_CONVERSION_RATIOS[]{
      0, 1. / 72, 1,
      5. / 127,  // 127/5 = 25.4, and 1in = 25.4mm
  };

  if constexpr (TUnit == PIXEL)
    return _length;

  constexpr double RATIO = UNIT_CONVERSION_RATIOS[to_underlying(TUnit)];
  return length_px<TOutput>{static_cast<TOutput>(static_cast<TUnderlying>(_length) * (RATIO * _screen_dpi))};
}

#define HUT_DECLARE_LENGTH_VEC_ALIASES(SIZE, UNIT)                                                                     \
  using u8vec##SIZE##_##UNIT  = vec_##UNIT<SIZE, u8>;                                                                  \
  using i8vec##SIZE##_##UNIT  = vec_##UNIT<SIZE, i8>;                                                                  \
  using u16vec##SIZE##_##UNIT = vec_##UNIT<SIZE, u16>;                                                                 \
  using i16vec##SIZE##_##UNIT = vec_##UNIT<SIZE, i16>;                                                                 \
  using u32vec##SIZE##_##UNIT = vec_##UNIT<SIZE, u32>;                                                                 \
  using uvec##SIZE##_##UNIT   = vec_##UNIT<SIZE, u32>;                                                                 \
  using i32vec##SIZE##_##UNIT = vec_##UNIT<SIZE, i32>;                                                                 \
  using ivec##SIZE##_##UNIT   = vec_##UNIT<SIZE, i32>;                                                                 \
  using f32vec##SIZE##_##UNIT = vec_##UNIT<SIZE, f32>;                                                                 \
  using fvec##SIZE##_##UNIT   = vec_##UNIT<SIZE, f32>;                                                                 \
  using vec##SIZE##_##UNIT    = vec_##UNIT<SIZE, f32>;                                                                 \
  using f64vec##SIZE##_##UNIT = vec_##UNIT<SIZE, f64>;                                                                 \
  using dvec##SIZE##_##UNIT   = vec_##UNIT<SIZE, f64>;

#define HUT_DECLARE_LENGTH_VEC(UNIT)                                                                                   \
  template<length_t TSize, typename TUnderlying>                                                                       \
  using vec_##UNIT = vec<TSize, length_##UNIT<TUnderlying>>;                                                           \
  HUT_DECLARE_LENGTH_VEC_ALIASES(1, UNIT)                                                                              \
  HUT_DECLARE_LENGTH_VEC_ALIASES(2, UNIT)                                                                              \
  HUT_DECLARE_LENGTH_VEC_ALIASES(3, UNIT)                                                                              \
  HUT_DECLARE_LENGTH_VEC_ALIASES(4, UNIT)

HUT_DECLARE_LENGTH_VEC(px)
HUT_DECLARE_LENGTH_VEC(pt)
HUT_DECLARE_LENGTH_VEC(in)
HUT_DECLARE_LENGTH_VEC(mm)

template<length_t TSize, typename TUnderlying, unit TUnit, arithmetic TScalar>
constexpr auto operator*(const vec<TSize, length<TUnderlying, TUnit>> &_v, TScalar _s) {
  vec<TSize, length<std::common_type_t<TUnderlying, TScalar>, TUnit>> result{_v};
  for (length_t i = 0; i < TSize; i++)
    result[i] *= _s;
  return result;
}

template<length_t TSize, typename TUnderlying, unit TUnit, arithmetic TScalar>
constexpr auto operator*(TScalar _s, const vec<TSize, length<TUnderlying, TUnit>> &_v) {
  vec<TSize, length<std::common_type_t<TUnderlying, TScalar>, TUnit>> result{_v};
  for (length_t i = 0; i < TSize; i++)
    result[i] *= _s;
  return result;
}

template<length_t TSize, typename TUnderlying, unit TUnit, arithmetic TScalar>
constexpr auto operator/(const vec<TSize, length<TUnderlying, TUnit>> &_v, TScalar _s) {
  vec<TSize, length<std::common_type_t<TUnderlying, TScalar>, TUnit>> result{_v};
  for (length_t i = 0; i < TSize; i++)
    result[i] /= _s;
  return result;
}

template<length_t TSize, typename TUnderlying, unit TUnit, arithmetic TScalar>
constexpr auto operator<<(TScalar _s, const vec<TSize, length<TUnderlying, TUnit>> &_v) {
  vec<TSize, length<std::common_type_t<TUnderlying, TScalar>, TUnit>> result{_v};
  for (length_t i = 0; i < TSize; i++)
    result[i] <<= _s;
  return result;
}

template<length_t TSize, typename TUnderlying, unit TUnit, arithmetic TScalar>
constexpr auto operator>>(const vec<TSize, length<TUnderlying, TUnit>> &_v, TScalar _s) {
  vec<TSize, length<std::common_type_t<TUnderlying, TScalar>, TUnit>> result{_v};
  for (length_t i = 0; i < TSize; i++)
    result[i] >>= _s;
  return result;
}

}  // namespace hut
