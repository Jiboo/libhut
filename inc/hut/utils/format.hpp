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

enum class format_suffix : u8 { UNORM, SNORM, USCALED, SSCALED, UINT, SINT, SFLOAT, UFLOAT, SRGB };

enum class format_target : u8 {
  COLOR,
  DEPTH,
  STENCIL,
  DEPTH_STENCIL,
};

struct format_info {
  u8 component_bits_[4];

  u8            components_;
  format_suffix suffix_;
  format_target target_;
  u8            pack_;

  static constexpr format_info from(VkFormat);

  [[nodiscard]] constexpr bool valid() const { return components_ > 0; }
  [[nodiscard]] constexpr u8   components_bits() const {
    u8 total = 0;
    for (u8 channel = 0; channel < components_; channel++)
      total += component_bits_[channel];
    assert(total > 0);
    return total;
  }
  [[nodiscard]] constexpr u8 max_component_bits() const {
    u8 max = 0;
    for (u8 channel = 0; channel < components_; channel++)
      max = std::max(max, component_bits_[channel]);
    assert(max > 0);
    return max;
  }
  [[nodiscard]] constexpr bool packed() const { return pack_ > 0 && components_bits() == pack_; }
  [[nodiscard]] constexpr bool compressed() const { return pack_ > 0 && components_bits() > pack_; }
  [[nodiscard]] constexpr u8   bpp() const { return compressed() ? pack_ : components_bits(); }
  [[nodiscard]] constexpr bool srgb() const { return suffix_ == format_suffix::SRGB; }
  [[nodiscard]] constexpr bool channels() const { return components_; }

  auto operator<=>(const format_info &) const = default;
};

constexpr format_info invalid_format_info{{0, 0, 0, 0}, 0, format_suffix::UNORM, format_target::COLOR, 0};
static_assert(!invalid_format_info.valid());

constexpr format_info format_info::from(VkFormat _input) {
  switch (_input) {
    case VK_FORMAT_R4G4_UNORM_PACK8: return format_info{{4, 4, 0, 0}, 2, format_suffix::UNORM, format_target::COLOR, 8};
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
      return format_info{{4, 4, 4, 4}, 4, format_suffix::UNORM, format_target::COLOR, 16};
    case VK_FORMAT_R5G6B5_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
      return format_info{{5, 6, 5, 0}, 3, format_suffix::UNORM, format_target::COLOR, 16};
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
      return format_info{{5, 5, 5, 1}, 4, format_suffix::UNORM, format_target::COLOR, 16};

    case VK_FORMAT_R8_UNORM: return format_info{{8, 0, 0, 0}, 1, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_R8_SNORM: return format_info{{8, 0, 0, 0}, 1, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_R8_USCALED: return format_info{{8, 0, 0, 0}, 1, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_R8_SSCALED: return format_info{{8, 0, 0, 0}, 1, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_R8_UINT: return format_info{{8, 0, 0, 0}, 1, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R8_SINT: return format_info{{8, 0, 0, 0}, 1, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R8_SRGB: return format_info{{8, 0, 0, 0}, 1, format_suffix::SRGB, format_target::COLOR, 0};

    case VK_FORMAT_R8G8_UNORM: return format_info{{8, 8, 0, 0}, 2, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_R8G8_SNORM: return format_info{{8, 8, 0, 0}, 2, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_R8G8_USCALED: return format_info{{8, 8, 0, 0}, 2, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_R8G8_SSCALED: return format_info{{8, 8, 0, 0}, 2, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_R8G8_UINT: return format_info{{8, 8, 0, 0}, 2, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R8G8_SINT: return format_info{{8, 8, 0, 0}, 2, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R8G8_SRGB: return format_info{{8, 8, 0, 0}, 2, format_suffix::SRGB, format_target::COLOR, 0};

    case VK_FORMAT_R8G8B8_UNORM: return format_info{{8, 8, 8, 0}, 3, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8_SNORM: return format_info{{8, 8, 8, 0}, 3, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8_USCALED: return format_info{{8, 8, 8, 0}, 3, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8_SSCALED: return format_info{{8, 8, 8, 0}, 3, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8_UINT: return format_info{{8, 8, 8, 0}, 3, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8_SINT: return format_info{{8, 8, 8, 0}, 3, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8_SRGB: return format_info{{8, 8, 8, 0}, 3, format_suffix::SRGB, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8_UNORM: return format_info{{8, 8, 8, 0}, 3, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8_SNORM: return format_info{{8, 8, 8, 0}, 3, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8_USCALED: return format_info{{8, 8, 8, 0}, 3, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8_SSCALED: return format_info{{8, 8, 8, 0}, 3, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8_UINT: return format_info{{8, 8, 8, 0}, 3, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8_SINT: return format_info{{8, 8, 8, 0}, 3, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8_SRGB: return format_info{{8, 8, 8, 0}, 3, format_suffix::SRGB, format_target::COLOR, 0};

    case VK_FORMAT_R8G8B8A8_UNORM: return format_info{{8, 8, 8, 8}, 4, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8A8_SNORM: return format_info{{8, 8, 8, 8}, 4, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8A8_USCALED:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8A8_SSCALED:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8A8_UINT: return format_info{{8, 8, 8, 8}, 4, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8A8_SINT: return format_info{{8, 8, 8, 8}, 4, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R8G8B8A8_SRGB: return format_info{{8, 8, 8, 8}, 4, format_suffix::SRGB, format_target::COLOR, 0};

    case VK_FORMAT_B8G8R8A8_UNORM: return format_info{{8, 8, 8, 8}, 4, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8A8_SNORM: return format_info{{8, 8, 8, 8}, 4, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8A8_USCALED:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8A8_SSCALED:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8A8_UINT: return format_info{{8, 8, 8, 8}, 4, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8A8_SINT: return format_info{{8, 8, 8, 8}, 4, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_B8G8R8A8_SRGB: return format_info{{8, 8, 8, 8}, 4, format_suffix::SRGB, format_target::COLOR, 0};

    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::UNORM, format_target::COLOR, 32};
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::SNORM, format_target::COLOR, 32};
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::USCALED, format_target::COLOR, 32};
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::SSCALED, format_target::COLOR, 32};
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::UINT, format_target::COLOR, 32};
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::SINT, format_target::COLOR, 32};
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      return format_info{{8, 8, 8, 8}, 4, format_suffix::SRGB, format_target::COLOR, 32};

    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::UNORM, format_target::COLOR, 32};
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::SNORM, format_target::COLOR, 32};
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::USCALED, format_target::COLOR, 32};
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::SSCALED, format_target::COLOR, 32};
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::UINT, format_target::COLOR, 32};
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::SINT, format_target::COLOR, 32};

    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::UNORM, format_target::COLOR, 32};
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::SNORM, format_target::COLOR, 32};
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::USCALED, format_target::COLOR, 32};
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::SSCALED, format_target::COLOR, 32};
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::UINT, format_target::COLOR, 32};
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
      return format_info{{10, 10, 10, 2}, 4, format_suffix::SINT, format_target::COLOR, 32};

    case VK_FORMAT_R16_UNORM: return format_info{{16, 0, 0, 0}, 1, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_R16_SNORM: return format_info{{16, 0, 0, 0}, 1, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_R16_USCALED: return format_info{{16, 0, 0, 0}, 1, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_R16_SSCALED: return format_info{{16, 0, 0, 0}, 1, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_R16_UINT: return format_info{{16, 0, 0, 0}, 1, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R16_SINT: return format_info{{16, 0, 0, 0}, 1, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R16_SFLOAT: return format_info{{16, 0, 0, 0}, 1, format_suffix::SFLOAT, format_target::COLOR, 0};

    case VK_FORMAT_R16G16_UNORM: return format_info{{16, 16, 0, 0}, 2, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_R16G16_SNORM: return format_info{{16, 16, 0, 0}, 2, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_R16G16_USCALED:
      return format_info{{16, 16, 0, 0}, 2, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_R16G16_SSCALED:
      return format_info{{16, 16, 0, 0}, 2, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_R16G16_UINT: return format_info{{16, 16, 0, 0}, 2, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R16G16_SINT: return format_info{{16, 16, 0, 0}, 2, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R16G16_SFLOAT: return format_info{{16, 16, 0, 0}, 2, format_suffix::SFLOAT, format_target::COLOR, 0};

    case VK_FORMAT_R16G16B16_UNORM:
      return format_info{{16, 16, 16, 0}, 3, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16_SNORM:
      return format_info{{16, 16, 16, 0}, 3, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16_USCALED:
      return format_info{{16, 16, 16, 0}, 3, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16_SSCALED:
      return format_info{{16, 16, 16, 0}, 3, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16_UINT: return format_info{{16, 16, 16, 0}, 3, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16_SINT: return format_info{{16, 16, 16, 0}, 3, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16_SFLOAT:
      return format_info{{16, 16, 16, 0}, 3, format_suffix::SFLOAT, format_target::COLOR, 0};

    case VK_FORMAT_R16G16B16A16_UNORM:
      return format_info{{16, 16, 16, 16}, 4, format_suffix::UNORM, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16A16_SNORM:
      return format_info{{16, 16, 16, 16}, 4, format_suffix::SNORM, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16A16_USCALED:
      return format_info{{16, 16, 16, 16}, 4, format_suffix::USCALED, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16A16_SSCALED:
      return format_info{{16, 16, 16, 16}, 4, format_suffix::SSCALED, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16A16_UINT:
      return format_info{{16, 16, 16, 16}, 4, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16A16_SINT:
      return format_info{{16, 16, 16, 16}, 4, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R16G16B16A16_SFLOAT:
      return format_info{{16, 16, 16, 16}, 4, format_suffix::SFLOAT, format_target::COLOR, 0};

    case VK_FORMAT_R32_UINT: return format_info{{32, 0, 0, 0}, 1, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R32_SINT: return format_info{{32, 0, 0, 0}, 1, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R32_SFLOAT: return format_info{{32, 0, 0, 0}, 1, format_suffix::SFLOAT, format_target::COLOR, 0};
    case VK_FORMAT_R32G32_UINT: return format_info{{32, 32, 0, 0}, 2, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R32G32_SINT: return format_info{{32, 32, 0, 0}, 2, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R32G32_SFLOAT: return format_info{{32, 32, 0, 0}, 2, format_suffix::SFLOAT, format_target::COLOR, 0};
    case VK_FORMAT_R32G32B32_UINT: return format_info{{32, 32, 32, 0}, 3, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R32G32B32_SINT: return format_info{{32, 32, 32, 0}, 3, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R32G32B32_SFLOAT:
      return format_info{{32, 32, 32, 0}, 3, format_suffix::SFLOAT, format_target::COLOR, 0};
    case VK_FORMAT_R32G32B32A32_UINT:
      return format_info{{32, 32, 32, 32}, 4, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R32G32B32A32_SINT:
      return format_info{{32, 32, 32, 32}, 4, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R32G32B32A32_SFLOAT:
      return format_info{{32, 32, 32, 32}, 4, format_suffix::SFLOAT, format_target::COLOR, 0};

    case VK_FORMAT_R64_UINT: return format_info{{64, 0, 0, 0}, 1, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R64_SINT: return format_info{{64, 0, 0, 0}, 1, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R64_SFLOAT: return format_info{{64, 0, 0, 0}, 1, format_suffix::SFLOAT, format_target::COLOR, 0};
    case VK_FORMAT_R64G64_UINT: return format_info{{64, 64, 0, 0}, 2, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R64G64_SINT: return format_info{{64, 64, 0, 0}, 2, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R64G64_SFLOAT: return format_info{{64, 64, 0, 0}, 2, format_suffix::SFLOAT, format_target::COLOR, 0};
    case VK_FORMAT_R64G64B64_UINT: return format_info{{64, 64, 64, 0}, 3, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R64G64B64_SINT: return format_info{{64, 64, 64, 0}, 3, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R64G64B64_SFLOAT:
      return format_info{{64, 64, 64, 0}, 3, format_suffix::SFLOAT, format_target::COLOR, 0};
    case VK_FORMAT_R64G64B64A64_UINT:
      return format_info{{64, 64, 64, 64}, 4, format_suffix::UINT, format_target::COLOR, 0};
    case VK_FORMAT_R64G64B64A64_SINT:
      return format_info{{64, 64, 64, 64}, 4, format_suffix::SINT, format_target::COLOR, 0};
    case VK_FORMAT_R64G64B64A64_SFLOAT:
      return format_info{{64, 64, 64, 64}, 4, format_suffix::SFLOAT, format_target::COLOR, 0};

    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
      return format_info{{11, 11, 10, 0}, 3, format_suffix::UFLOAT, format_target::COLOR, 32};

    case VK_FORMAT_D16_UNORM: return format_info{{16, 0, 0, 0}, 1, format_suffix::UNORM, format_target::DEPTH, 0};
    case VK_FORMAT_D32_SFLOAT: return format_info{{32, 0, 0, 0}, 1, format_suffix::UNORM, format_target::DEPTH, 0};
    case VK_FORMAT_S8_UINT: return format_info{{8, 0, 0, 0}, 1, format_suffix::UNORM, format_target::STENCIL, 0};
    case VK_FORMAT_D16_UNORM_S8_UINT:
      return format_info{{16, 8, 0, 0}, 2, format_suffix::UNORM, format_target::DEPTH_STENCIL, 0};
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return format_info{{32, 8, 0, 0}, 2, format_suffix::UNORM, format_target::DEPTH_STENCIL, 0};

    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
      return format_info{{5, 6, 5, 0}, 3, format_suffix::UNORM, format_target::COLOR, 4};
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
      return format_info{{5, 6, 5, 0}, 3, format_suffix::SRGB, format_target::COLOR, 4};
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
      return format_info{{5, 6, 5, 1}, 4, format_suffix::UNORM, format_target::COLOR, 4};
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
      return format_info{{5, 6, 5, 1}, 4, format_suffix::SRGB, format_target::COLOR, 4};
    case VK_FORMAT_BC2_UNORM_BLOCK: return format_info{{5, 6, 5, 4}, 4, format_suffix::UNORM, format_target::COLOR, 8};
    case VK_FORMAT_BC2_SRGB_BLOCK: return format_info{{5, 6, 5, 4}, 4, format_suffix::SRGB, format_target::COLOR, 8};
    case VK_FORMAT_BC3_UNORM_BLOCK: return format_info{{5, 6, 5, 8}, 4, format_suffix::UNORM, format_target::COLOR, 8};
    case VK_FORMAT_BC3_SRGB_BLOCK: return format_info{{5, 6, 5, 8}, 4, format_suffix::SRGB, format_target::COLOR, 8};
    case VK_FORMAT_BC4_UNORM_BLOCK: return format_info{{8, 0, 0, 0}, 1, format_suffix::UNORM, format_target::COLOR, 4};
    case VK_FORMAT_BC4_SNORM_BLOCK: return format_info{{8, 0, 0, 0}, 1, format_suffix::SRGB, format_target::COLOR, 4};
    case VK_FORMAT_BC5_UNORM_BLOCK: return format_info{{8, 8, 0, 0}, 2, format_suffix::UNORM, format_target::COLOR, 8};
    case VK_FORMAT_BC5_SNORM_BLOCK: return format_info{{8, 8, 0, 0}, 2, format_suffix::SNORM, format_target::COLOR, 8};
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
      return format_info{{16, 16, 16, 0}, 3, format_suffix::UFLOAT, format_target::COLOR, 8};
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
      return format_info{{16, 16, 16, 0}, 3, format_suffix::SFLOAT, format_target::COLOR, 8};
    case VK_FORMAT_BC7_UNORM_BLOCK: return format_info{{8, 8, 8, 8}, 4, format_suffix::UNORM, format_target::COLOR, 8};
    case VK_FORMAT_BC7_SRGB_BLOCK: return format_info{{8, 8, 8, 8}, 4, format_suffix::SRGB, format_target::COLOR, 8};

    default: return invalid_format_info;
  }
}

}  // namespace hut
