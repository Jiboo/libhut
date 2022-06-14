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

#include "hut/utils/glm.hpp"
#include "hut/utils/length.hpp"

namespace hut {

template<unit TUnit, typename TUnderlying>
struct bbox : vec<4, length<TUnit, TUnderlying>> {
  template<typename TOtherUnderlying>
  using common_type_t = std::common_type_t<TUnderlying, TOtherUnderlying>;
  template<typename TOtherUnderlying>
  using common_length_t = length<TUnit, common_type_t<TOtherUnderlying>>;
  template<typename TOtherUnderlying>
  using common_vec4_t = vec<4, common_length_t<TOtherUnderlying>>;
  template<typename TOtherUnderlying>
  using common_vec2_t = vec<2, common_length_t<TOtherUnderlying>>;
  template<typename TOtherUnderlying>
  using common_t = bbox<TUnit, common_type_t<TOtherUnderlying>>;

  template<typename TOtherUnderlying>
  using compat_length_t = length<TUnit, TOtherUnderlying>;
  template<typename TOtherUnderlying>
  using compat_vec4_t = vec<4, compat_length_t<TOtherUnderlying>>;
  template<typename TOtherUnderlying>
  using compat_vec2_t = vec<2, compat_length_t<TOtherUnderlying>>;
  template<typename TOtherUnderlying>
  using compat_t = bbox<TUnit, TOtherUnderlying>;

  using length_t = compat_length_t<TUnderlying>;
  using vec4_t   = compat_vec4_t<TUnderlying>;
  using vec2_t   = compat_vec2_t<TUnderlying>;

  using vec<4, length<TUnit, TUnderlying>>::vec;

  [[nodiscard]] constexpr static bbox with_min_max(const vec2_t &_min, const vec2_t &_max) { return {_min, _max}; }
  [[nodiscard]] constexpr static bbox with_origin_size(const vec2_t &_o, const vec2_t &_s) { return {_o, _o + _s}; }

  [[nodiscard]] constexpr const length_t &left() const { return this->x; }
  [[nodiscard]] constexpr const length_t &top() const { return this->y; }
  [[nodiscard]] constexpr const length_t &right() const { return this->z; }
  [[nodiscard]] constexpr const length_t &bottom() const { return this->w; }

  [[nodiscard]] constexpr length_t &left() { return this->x; }
  [[nodiscard]] constexpr length_t &top() { return this->y; }
  [[nodiscard]] constexpr length_t &right() { return this->z; }
  [[nodiscard]] constexpr length_t &bottom() { return this->w; }

  [[nodiscard]] constexpr vec2_t origin() const { return {left(), top()}; }
  [[nodiscard]] constexpr vec2_t size() const { return {right() - left(), bottom() - top()}; }
  [[nodiscard]] constexpr vec2_t center() const {
    auto s = size() / 2;
    return {left() + s.x, top() + s.y};
  }

  [[nodiscard]] constexpr length_t width() const { return size().x; }
  [[nodiscard]] constexpr length_t height() const { return size().y; }

  [[nodiscard]] constexpr vec2_t min() const { return {left(), top()}; }
  [[nodiscard]] constexpr vec2_t max() const { return {right(), bottom()}; }

  template<typename TPointUnderlying>
  constexpr bool contains(const compat_vec2_t<TPointUnderlying> &_point) {
    return _point.x >= left() && _point.x <= right() && _point.y >= top() && _point.y <= bottom();
  }
  template<typename TPointUnderlying>
  constexpr bool strictly_contains(const compat_vec2_t<TPointUnderlying> &_point) {
    return _point.x > left() && _point.x < right() && _point.y > top() && _point.y < bottom();
  }

  template<typename TOtherUnderlying>
  constexpr common_vec4_t<TOtherUnderlying> cut_left(compat_length_t<TOtherUnderlying> _l) {
    assert(width() >= _l);
    const u16_px prev_left = left();
    left() += _l;
    return {prev_left, top(), left(), bottom()};
  }

  template<typename TOtherUnderlying>
  constexpr common_vec4_t<TOtherUnderlying> cut_top(compat_length_t<TOtherUnderlying> _l) {
    assert(height() >= _l);
    auto prev_top = top();
    top() += _l;
    return {left(), prev_top, right(), top()};
  }

  template<typename TOtherUnderlying>
  constexpr common_vec4_t<TOtherUnderlying> cut_right(compat_length_t<TOtherUnderlying> _l) {
    assert(width() >= _l);
    auto prev_right = right();
    right() -= _l;
    return {right(), top(), prev_right, bottom()};
  }

  template<typename TOtherUnderlying>
  constexpr common_vec4_t<TOtherUnderlying> cut_bottom(compat_length_t<TOtherUnderlying> _l) {
    assert(height() >= _l);
    auto prev_bottom = bottom();
    bottom() -= _l;
    return {left(), bottom(), right(), prev_bottom};
  }
};

#define HUT_DECLARE_BBOX_NUMERIC(TYPE, NAME, SUFFIX) using NAME##bbox_##SUFFIX = bbox_##SUFFIX##_t<TYPE>;

#define HUT_DECLARE_BBOX(SUFFIX, ENUMERATOR)                                                                           \
  template<typename TUnderlying>                                                                                       \
  using bbox_##SUFFIX##_t = bbox<ENUMERATOR, TUnderlying>;                                                             \
  HUT_FOR_EACH_NUMERIC(HUT_DECLARE_BBOX_NUMERIC, SUFFIX)

HUT_FOR_EACH_UNIT(HUT_DECLARE_BBOX)

HUT_SCALAR_OPERATORS(HUT_PACK(unit TUnit, typename TUnderlying), HUT_PACK(bbox<TUnit, TUnderlying>),
                     HUT_PACK(typename bbox<TUnit, TUnderlying>::template common_t<TScalar>))

}  // namespace hut
