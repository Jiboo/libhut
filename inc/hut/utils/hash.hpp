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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/string_cast.hpp>

namespace hut {

inline void hash_combine(std::size_t &_seed) {
}

template<typename TType, typename... TRest>
inline void hash_combine(std::size_t &_seed, const TType &_v, TRest &&..._rest) {
  std::hash<TType> hasher;
  _seed ^= hasher(_v) + 0x9e3779b9 + (_seed << 6U) + (_seed >> 2U);
  hash_combine(_seed, std::forward<TRest>(_rest)...);
}

template<typename TOut, typename TIn>
static TOut rehash(const TIn &_in) {
  static_assert(sizeof(TIn) > sizeof(TOut));
  static_assert(sizeof(TIn) % sizeof(TOut) == 0);
  constexpr auto RATIO = sizeof(TIn) / sizeof(TOut);

  std::span<const TOut, RATIO> in(reinterpret_cast<const TOut *>(&_in), RATIO);
  TOut                         result{};
  for (size_t i = 0; i < RATIO; i++)
    result ^= in[i];
  return result;
}

}  // namespace hut
