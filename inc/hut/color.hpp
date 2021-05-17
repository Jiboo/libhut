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

#include <cassert>

#include "hut/utils.hpp"

namespace hut {

inline float format_bpp(VkFormat _format) {
  switch (_format) {
    case VK_FORMAT_R8_UNORM: return 1;

    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SRGB: return 2;

    case VK_FORMAT_R8G8B8_UNORM: return 3;

    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_D32_SFLOAT: return 4;

    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:return 0.5;

    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK: return 1; // 16px in 16bytes

    default:
      assert(false);
      return 0;
  }
}

inline u8vec2 format_block_size(VkFormat _format) {
  switch (_format) {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_D32_SFLOAT: return {1, 1};

    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK: return {4, 4};

    default:
      assert(false);
      return {1, 1};
  }
}

}  // namespace hut
