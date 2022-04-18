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

#include <vulkan/vulkan.h>

#include "hut/utils/math.hpp"

namespace hut {

inline i16vec2_px offset2_16(VkOffset2D _in) {
  return i16vec2_px{_in.x, _in.y};
}
inline i16vec3_px offset3_16(VkOffset3D _in) {
  return i16vec3_px{_in.x, _in.y, _in.z};
}
inline u16vec2_px extent2_16(VkExtent2D _in) {
  return u16vec2_px{_in.width, _in.height};
}
inline u16vec3_px extent3_16(VkExtent3D _in) {
  return u16vec3_px{_in.width, _in.height, _in.depth};
}
inline f32vec4_rgba color4_32(VkClearColorValue _in) {
  return f32vec4_rgba{_in.float32[0], _in.float32[1], _in.float32[2], _in.float32[3]};
}

}  // namespace hut
