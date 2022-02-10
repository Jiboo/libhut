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
#include "hut/utils/string.hpp"

namespace hut {

template<typename TUnderlying, fixed_string TUnit>
struct boxed_number {
  using type                  = TUnderlying;
  constexpr static char *UNIT = TUnit.data_;

  template<typename TOtherUnderlying>
  using common = boxed_number<std::common_type_t<TUnderlying, TOtherUnderlying>, TUnit>;
  template<typename TOtherUnderlying>
  using same = boxed_number<TOtherUnderlying, TUnit>;

  TUnderlying value_;

  constexpr boxed_number() = default;
  constexpr explicit boxed_number(TUnderlying _raw)
      : value_(_raw) {}
  constexpr boxed_number(const boxed_number &_other)
      : value_(_other.value_) {}
  template<typename TOtherUnderlying>
  constexpr boxed_number(same<TOtherUnderlying> _o)
      : value_(_o.value_) {}
  constexpr ~boxed_number() = default;

  constexpr boxed_number &operator=(const boxed_number &_o) = default;
  template<typename TOtherUnderlying>
  constexpr boxed_number &operator=(same<TOtherUnderlying> _o) {
    value_ = _o.value_;
    return *this;
  }

  template<typename TOtherUnderlying>
  constexpr boxed_number &operator+=(same<TOtherUnderlying> _o) {
    value_ += _o.value_;
    return *this;
  }
  template<typename TOtherUnderlying>
  constexpr boxed_number &operator-=(same<TOtherUnderlying> _o) {
    value_ -= _o.value_;
    return *this;
  }
  template<typename TOtherUnderlying>
  constexpr boxed_number &operator*=(TOtherUnderlying _o) {
    value_ *= _o;
    return *this;
  }
  template<typename TOtherUnderlying>
  constexpr boxed_number &operator/=(TOtherUnderlying _o) {
    value_ /= _o;
    return *this;
  }

  template<typename TOtherUnderlying>
  constexpr bool operator==(same<TOtherUnderlying> _o) const {
    return value_ == _o.value_;
  }
  template<typename TOtherUnderlying>
  constexpr bool operator!=(same<TOtherUnderlying> _o) const {
    return value_ != _o.value_;
  }
  template<typename TOtherUnderlying>
  constexpr bool operator<=(same<TOtherUnderlying> _o) const {
    return value_ <= _o.value_;
  }
  template<typename TOtherUnderlying>
  constexpr bool operator>=(same<TOtherUnderlying> _o) const {
    return value_ >= _o.value_;
  }
  template<typename TOtherUnderlying>
  constexpr bool operator<(same<TOtherUnderlying> _o) const {
    return value_ < _o.value_;
  }
  template<typename TOtherUnderlying>
  constexpr bool operator>(same<TOtherUnderlying> _o) const {
    return value_ > _o.value_;
  }

  template<typename TOtherUnderlying>
  constexpr common<TOtherUnderlying> operator+(const same<TOtherUnderlying> &_o) const {
    std::common_type_t<TUnderlying, TOtherUnderlying> res = value_ + _o.value_;
    return common<TOtherUnderlying>{res};
  }
  template<typename TOtherUnderlying>
  constexpr common<TOtherUnderlying> operator-(const same<TOtherUnderlying> &_o) const {
    std::common_type_t<TUnderlying, TOtherUnderlying> res = value_ + _o.value_;
    return common<TOtherUnderlying>{res};
  }
  template<typename TOtherUnderlying>
  constexpr common<TOtherUnderlying> operator*(const TOtherUnderlying &_o) const {
    return common<TOtherUnderlying>{value_ * _o};
  }
  template<typename TOtherUnderlying>
  constexpr common<TOtherUnderlying> operator/(const TOtherUnderlying &_o) const {
    return common<TOtherUnderlying>{value_ / _o};
  }
};

template<typename TUnderlying>
using size_px = boxed_number<TUnderlying, "px">;

template<typename TUnderlying, typename TDpiRati, fixed_string TUnit>
struct scalable_size : boxed_number<TUnderlying, TUnit> {
  using boxed_number<TUnderlying, TUnit>::boxed_number;

  template<typename TOutput = f32>
  constexpr size_px<TOutput> scale(uint _screen_dpi) {
    constexpr double RATIO = static_cast<double>(TDpiRati::num) / TDpiRati::den;
    return size_px<TOutput>{static_cast<TOutput>(this->value_ * (RATIO * _screen_dpi))};
  }
};

template<typename TUnderlying>
using size_pt = scalable_size<TUnderlying, std::ratio<1, 72>, "pt">;
template<typename TUnderlying>
using size_in = scalable_size<TUnderlying, std::ratio<1, 1>, "in">;
template<typename TUnderlying>
using size_mm = scalable_size<TUnderlying, std::ratio<5, 127>, "mm">;  // 127/5 = 25.4, and 1in = 25.4mm

constexpr inline size_px<long long unsigned> operator"" _px(long long unsigned _in) {
  return size_px<long long unsigned>{_in};
}
constexpr inline size_pt<long double> operator"" _pt(long double _in) {
  return size_pt<long double>{_in};
}
constexpr inline size_in<long double> operator"" _in(long double _in) {
  return size_in<long double>{_in};
}
constexpr inline size_mm<long double> operator"" _mm(long double _in) {
  return size_mm<long double>{_in};
}

}  // namespace hut
