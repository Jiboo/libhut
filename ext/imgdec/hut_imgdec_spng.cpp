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

#ifdef HUT_ENABLE_IMGDEC_LIBSPNG

#include "hut_imgdec.hpp"

#include "spng.h"

namespace hut::imgdec {

  std::shared_ptr<image> load_png(display &_display, std::span<const u8> _data) {
    auto ctx = std::unique_ptr<spng_ctx, decltype(&spng_ctx_free)>(spng_ctx_new(0), spng_ctx_free);
    spng_set_png_buffer(ctx.get(), _data.data(), _data.size_bytes());

    spng_ihdr ihdr = {};
    if (spng_get_ihdr(ctx.get(), &ihdr) != SPNG_OK)
      return nullptr;

    uint8_t rendering_intent = 0;
    bool has_srgb = spng_get_srgb(ctx.get(), &rendering_intent) == 0;

    spng_format decode_format;
    switch (ihdr.color_type) {
      case spng_color_type::SPNG_COLOR_TYPE_GRAYSCALE:
        decode_format = SPNG_FMT_G8;
        break;
      case spng_color_type::SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:
        decode_format = (ihdr.bit_depth == 16) ? SPNG_FMT_GA16 : SPNG_FMT_GA8;
        break;
      case spng_color_type::SPNG_COLOR_TYPE_INDEXED: [[fallthrough]];
      case spng_color_type::SPNG_COLOR_TYPE_TRUECOLOR: [[fallthrough]];
      case spng_color_type::SPNG_COLOR_TYPE_TRUECOLOR_ALPHA:
        decode_format = (ihdr.bit_depth == 16) ? SPNG_FMT_RGBA16 : SPNG_FMT_RGBA8;
        break;
    }

    image_params iparams;
    switch (decode_format) {
      case SPNG_FMT_RGBA8: iparams.format_ = has_srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; break;
      case SPNG_FMT_RGBA16: iparams.format_ = VK_FORMAT_R16G16B16A16_UNORM; break;
      case SPNG_FMT_GA8: iparams.format_ = VK_FORMAT_R8G8_UNORM; break;
      case SPNG_FMT_GA16: iparams.format_ = VK_FORMAT_R16G16_UNORM; break;
      case SPNG_FMT_G8: iparams.format_ = VK_FORMAT_R8_UNORM; break;
      default: return nullptr;
    }

    assert(ihdr.width < _display.limits().maxImageDimension2D);
    assert(ihdr.height < _display.limits().maxImageDimension2D);
    assert(ihdr.width < (1<<16));
    assert(ihdr.height < (1<<16));
    iparams.size_ = {ihdr.width, ihdr.height};

    auto result = std::make_shared<image>(_display, iparams);
    if (!result)
      return nullptr;

    auto updator = result->prepare_update();
    if (spng_decode_image(ctx.get(), updator.data(), updator.size_bytes(),
                          SPNG_FMT_RGBA8, SPNG_DECODE_PROGRESSIVE) != SPNG_OK)
      return nullptr;

    auto img_row_pitch = updator.image_row_pitch();
    for (uint row = 0; row < ihdr.height; row++) {
      u8 *target = updator.data() + (row * updator.staging_row_pitch());
      auto errcode = spng_decode_row(ctx.get(), target, img_row_pitch);
      if (errcode != SPNG_OK && errcode != SPNG_EOI)
        return nullptr;
    }

    return result;
  }

} // ns hut::imgdec

#endif  // HUT_ENABLE_IMGDEC_LIBSPNG
