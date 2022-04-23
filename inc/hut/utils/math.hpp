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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/string_cast.hpp>

namespace hut {

using namespace glm;

template<typename TType>
constexpr inline TType NUMAX = std::numeric_limits<TType>::max();

template<typename TType>
constexpr inline TType NUMIN = std::numeric_limits<TType>::min();

template<typename TEnum>
constexpr std::underlying_type_t<TEnum> to_underlying(TEnum _val) noexcept {
  return static_cast<std::underlying_type_t<TEnum>>(_val);
}

template<typename TIn>
concept arithmetic = std::is_arithmetic_v<TIn>;

template<length_t TSize, arithmetic TVec, arithmetic TScalar>
  requires(!std::is_same_v<TVec, TScalar>)
constexpr auto operator*(vec<TSize, TVec> _v, TScalar _s) {
  return _v *= _s;
}

template<length_t TSize, arithmetic TVec, arithmetic TScalar>
  requires(!std::is_same_v<TVec, TScalar>)
constexpr auto operator*(TScalar _s, vec<TSize, TVec> _v) {
  return _v *= _s;
}

template<length_t TSize, arithmetic TVec, arithmetic TScalar>
  requires(!std::is_same_v<TVec, TScalar>)
constexpr auto operator/(vec<TSize, TVec> _v, TScalar _s) {
  return _v /= _s;
}

template<typename TType, TType... TValues>
constexpr TType max() {
  TType values[] = {TValues...};
  TType result   = values[0];
  for (size_t i = 0; i < sizeof...(TValues); i++)
    result = std::max(result, values[i]);
  return result;
}

template<typename TType, TType... TValues>
constexpr TType sum() {
  TType values[] = {TValues...};
  TType result   = 0;
  for (size_t i = 0; i < sizeof...(TValues); i++)
    result += values[i];
  return result;
}

template<typename TType>
constexpr TType align(TType _input, unsigned _align) {
  if (_align == 0)
    return _input;

  TType rest = _input % _align;
  return _input + (rest ? (_align - rest) : 0);
}

template<typename TType, qualifier TQualifier = defaultp>
inline vec<2, TType, TQualifier> bbox_origin(const vec<4, TType, TQualifier> &_input) {
  return vec<2, TType, TQualifier>{_input[0], _input[1]};
}

template<typename TType, qualifier TQualifier = defaultp>
inline vec<2, TType, TQualifier> bbox_size(const vec<4, TType, TQualifier> &_input) {
  assert(_input[2] >= _input[0]);
  assert(_input[3] >= _input[1]);
  return vec<2, TType, TQualifier>{_input[2] - _input[0], _input[3] - _input[1]};
}

template<typename TType, qualifier TQualifier = defaultp>
inline vec<2, TType, TQualifier> bbox_center(const vec<4, TType, TQualifier> &_input) {
  const auto size = bbox_size(_input);
  return vec<2, TType, TQualifier>{_input[0] + size.x / 2, _input[1] + size.y / 2};
}

template<typename TBox, typename TPoint, qualifier TQualifier = defaultp>
bool bbox_contains(const vec<4, TBox, TQualifier> &_box, const vec<2, TPoint, TQualifier> &_point) {
  return _point.x >= _box.x && _point.x <= _box.z && _point.y >= _box.y && _point.y <= _box.w;
}

template<typename TType, qualifier TQualifier = defaultp>
inline vec<4, TType, TQualifier> make_bbox_with_origin_size(const vec<2, TType, TQualifier> &_offset,
                                                            const vec<2, TType, TQualifier> &_size) {
  return vec<4, TType, TQualifier>{_offset.x, _offset.y, _offset.x + _size.x, _offset.y + _size.y};
}

inline mat4 make_transform_mat4(vec2 _translate) {
  mat4 m(1);
  m = translate(m, vec3(_translate, 0));
  return m;
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

}  // namespace hut
