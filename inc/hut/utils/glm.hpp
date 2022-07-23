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
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/string_cast.hpp>

#include "hut/utils/math.hpp"
#include "hut/utils/preprocessor.hpp"

namespace hut {

using namespace glm;

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

#define HUT_FOR_EACH_NUMERIC(XMACRO, ...)                                                                              \
  XMACRO(u8, u8 __VA_OPT__(, ) __VA_ARGS__)                                                                            \
  XMACRO(i8, i8 __VA_OPT__(, ) __VA_ARGS__)                                                                            \
  XMACRO(u16, u16 __VA_OPT__(, ) __VA_ARGS__)                                                                          \
  XMACRO(i16, i16 __VA_OPT__(, ) __VA_ARGS__)                                                                          \
  XMACRO(u32, u32 __VA_OPT__(, ) __VA_ARGS__)                                                                          \
  XMACRO(u32, u __VA_OPT__(, ) __VA_ARGS__)                                                                            \
  XMACRO(i32, i32 __VA_OPT__(, ) __VA_ARGS__)                                                                          \
  XMACRO(i32, i __VA_OPT__(, ) __VA_ARGS__)                                                                            \
  XMACRO(f32, f32 __VA_OPT__(, ) __VA_ARGS__)                                                                          \
  XMACRO(f32, f __VA_OPT__(, ) __VA_ARGS__)                                                                            \
  XMACRO(f32, __VA_OPT__(, ) __VA_ARGS__)                                                                              \
  XMACRO(u64, u64 __VA_OPT__(, ) __VA_ARGS__)                                                                          \
  XMACRO(i64, i64 __VA_OPT__(, ) __VA_ARGS__)                                                                          \
  XMACRO(f64, f64 __VA_OPT__(, ) __VA_ARGS__)                                                                          \
  XMACRO(f64, d __VA_OPT__(, ) __VA_ARGS__)

#define HUT_SCALAR_OPERATOR_RIGHT(OP, TPARAMS, ITYPE, OTYPE)                                                           \
  template<HUT_PACK_WITH_OPT_COMMA(TPARAMS) arithmetic TScalar>                                                        \
  constexpr auto operator OP(const HUT_PACK(ITYPE) & _in, TScalar _s) {                                                \
    HUT_PACK(OTYPE) result{_in};                                                                                       \
    for (uint i = 0; i < _in.length(); i++)                                                                            \
      result[i] OP## = _s;                                                                                             \
    return result;                                                                                                     \
  }

#define HUT_SCALAR_OPERATOR_LEFT(OP, TPARAMS, ITYPE, OTYPE)                                                            \
  template<HUT_PACK_WITH_OPT_COMMA(TPARAMS) arithmetic TScalar>                                                        \
  constexpr auto operator OP(TScalar _s, const HUT_PACK(ITYPE) & _in) {                                                \
    HUT_PACK(OTYPE) result{_in};                                                                                       \
    for (uint i = 0; i < _in.length(); i++)                                                                            \
      result[i] OP## = _s;                                                                                             \
    return result;                                                                                                     \
  }

#define HUT_SCALAR_OPERATORS(TPARAMS, ITYPE, OTYPE)                                                                    \
  HUT_SCALAR_OPERATOR_LEFT(*, HUT_PACK(TPARAMS), HUT_PACK(ITYPE), HUT_PACK(OTYPE))                                     \
  HUT_SCALAR_OPERATOR_RIGHT(*, HUT_PACK(TPARAMS), HUT_PACK(ITYPE), HUT_PACK(OTYPE))                                    \
  HUT_SCALAR_OPERATOR_RIGHT(/, HUT_PACK(TPARAMS), HUT_PACK(ITYPE), HUT_PACK(OTYPE))                                    \
  HUT_SCALAR_OPERATOR_RIGHT(>>, HUT_PACK(TPARAMS), HUT_PACK(ITYPE), HUT_PACK(OTYPE))                                   \
  HUT_SCALAR_OPERATOR_RIGHT(<<, HUT_PACK(TPARAMS), HUT_PACK(ITYPE), HUT_PACK(OTYPE))

}  // namespace hut
