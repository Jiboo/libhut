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

#include <cstring>

#include <bit>

#include "hut/ktx2/ktx2.hpp"

namespace hut::ktx {

bool is_prohibited_format(VkFormat _format) {
  // http://github.khronos.org/KTX-Specification/#prohibitedFormats
  switch (_format) {
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
    case VK_FORMAT_R8_USCALED:
    case VK_FORMAT_R8_SSCALED:
    case VK_FORMAT_R8G8_USCALED:
    case VK_FORMAT_R8G8_SSCALED:
    case VK_FORMAT_R8G8B8_USCALED:
    case VK_FORMAT_R8G8B8_SSCALED:
    case VK_FORMAT_B8G8R8_USCALED:
    case VK_FORMAT_B8G8R8_SSCALED:
    case VK_FORMAT_R8G8B8A8_USCALED:
    case VK_FORMAT_R8G8B8A8_SSCALED:
    case VK_FORMAT_B8G8R8A8_USCALED:
    case VK_FORMAT_B8G8R8A8_SSCALED:
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
    case VK_FORMAT_R16_USCALED:
    case VK_FORMAT_R16_SSCALED:
    case VK_FORMAT_R16G16_USCALED:
    case VK_FORMAT_R16G16_SSCALED:
    case VK_FORMAT_R16G16B16_USCALED:
    case VK_FORMAT_R16G16B16_SSCALED:
    case VK_FORMAT_R16G16B16A16_USCALED:
    case VK_FORMAT_R16G16B16A16_SSCALED:
    case VK_FORMAT_G8B8G8R8_422_UNORM:
    case VK_FORMAT_B8G8R8G8_422_UNORM:
    case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
    case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
    case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
    case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
    case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
    case VK_FORMAT_R10X6_UNORM_PACK16:
    case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
    case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
    case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
    case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
    case VK_FORMAT_R12X4_UNORM_PACK16:
    case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
    case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
    case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
    case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
    case VK_FORMAT_G16B16G16R16_422_UNORM:
    case VK_FORMAT_B16G16R16G16_422_UNORM:
    case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
    case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
    case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
    case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
    case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
      return true;
    default:
      return false;
  }
}

bool is_special_depth_format(VkFormat _format) {
  switch (_format) {
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return true;
    default:
      return false;
  }
}

std::optional<shared_image> load(display &_display, std::span<const u8> _input, const load_params &_params) {
  static_assert(std::endian::native == std::endian::little);

  struct level_ranges {
    u64 byte_offset_;
    u64 byte_length_;
    u64 uncompressed_byte_length_;
  };

  if (_input.empty())
    return {};

  image_params iparams;
  level_ranges levels[32];
  u32          type_size;

  {
    iparams.tiling_     = _params.tiling_;
    iparams.usage_      = _params.usage_;
    iparams.aspect_     = _params.aspect_;
    iparams.properties_ = _params.properties_;
    iparams.samples_    = _params.samples_;
    iparams.flags_      = _params.flags_;

    const u8 *start = _input.begin().base();
    const u8 *end   = _input.end().base();
    const u8 *pos   = start;

    auto read_u32 = [&]() -> u32 {
      assert(pos + 4 <= end);
      u32 result = pos[0] << 0 | pos[1] << 8 | pos[2] << 16 | pos[3] << 24;
      pos += 4;
      return result;
    };

    auto read_u64 = [&]() -> u64 {
      assert(pos + 4 <= end);
      u64 left  = read_u32();
      u64 right = read_u32();
      return left << 0 | right << 32;
    };

    auto expect_data = [&](const auto *_expected, size_t _byte_size) -> bool {
      assert(pos + _byte_size <= end);
      if (memcmp(_expected, pos, _byte_size) != 0)
        return false;
      pos += _byte_size;
      return true;
    };

    constexpr static u8 zero_padding_[4] = {
        0, 0, 0, 0};
    constexpr static u8 expected_ktx2_header_[12] = {
        0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
    if (!expect_data(expected_ktx2_header_, 12))
      return {};

    iparams.format_ = static_cast<VkFormat>(read_u32());
    if (is_prohibited_format(iparams.format_))
      return {};
    if (is_special_depth_format(iparams.format_))
      return {};  // FIXME http://github.khronos.org/KTX-Specification/#_depth_and_stencil_formats
    if (iparams.format_ == VK_FORMAT_UNDEFINED)
      return {};  // FIXME http://github.khronos.org/KTX-Specification/#_use_of_vk_format_undefined

    type_size       = read_u32();
    iparams.size_.x = read_u32();
    iparams.size_.y = read_u32();
    u32 pixel_depth = read_u32();
    if (pixel_depth > 1)
      return {};  // FIXME Support 3D texture
    iparams.layers_ = max(read_u32(), u32{1});
    u32 face_count  = read_u32();
    if (face_count != 1 && face_count != 6)
      return {};
    if (iparams.layers_ != 1 && face_count != 1)
      return {};
    if (face_count == 6) {
      iparams.layers_ = 6;
      iparams.flags_ |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    iparams.levels_ = read_u32();
    u32 compression = read_u32();
    if (compression != 0)
      return {};  // FIXME Support some compression

    u32 dfd_byte_offset = read_u32();
    u32 dfd_byte_length = read_u32();
    u32 kvd_byte_offset = read_u32();
    u32 kvd_byte_length = read_u32();
    u64 sgd_byte_offset = read_u64();
    u64 sgd_byte_length = read_u64();

    for (uint level = 0; level < iparams.levels_; level++) {
      auto &range                     = levels[level];
      range.byte_offset_              = read_u64();
      range.byte_length_              = read_u64();
      range.uncompressed_byte_length_ = read_u64();
    }
  }

  shared_image img = std::make_shared<image>(_display, iparams);
  for (u16 level = 0; level < iparams.levels_; level++) {
    const auto &level_range = levels[level];
    u16vec2     level_size  = iparams.size_ >> level;
    u32         bit_stride  = level_size.x * img->bpp();
    assert(bit_stride > 8);
    u32  byte_stride       = bit_stride / 8;
    auto layer_byte_length = byte_stride * level_size.y;
    assert(level_range.uncompressed_byte_length_ == layer_byte_length * iparams.layers_);

    for (u16 layer = 0; layer < iparams.layers_; layer++) {
      auto layer_byte_offset = _input.data() + level_range.byte_offset_ + layer_byte_length * layer;
      auto subres            = image::subresource{u16vec4{0, 0, level_size}, level, layer};
      img->update(subres, std::span<const u8>(layer_byte_offset, layer_byte_length), byte_stride);
    }
  }

  return img;
}

}  // namespace hut::ktx
