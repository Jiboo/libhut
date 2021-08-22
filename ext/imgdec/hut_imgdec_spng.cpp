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

  bool prepare_read(image_params *_params, spng_format *_format, spng_ctx &_ctx, std::span<const u8> _data) {
    image_params params;
    spng_format format;

    spng_set_png_buffer(&_ctx, _data.data(), _data.size_bytes());

    spng_ihdr ihdr = {};
    if (spng_get_ihdr(&_ctx, &ihdr) != SPNG_OK)
      return false;

    uint8_t rendering_intent = 0;
    bool has_srgb = spng_get_srgb(&_ctx, &rendering_intent) == 0;

    switch (ihdr.color_type) {
      case spng_color_type::SPNG_COLOR_TYPE_GRAYSCALE:
        format = SPNG_FMT_G8;
        break;
      case spng_color_type::SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:
        format = (ihdr.bit_depth == 16) ? SPNG_FMT_GA16 : SPNG_FMT_GA8;
        break;
      case spng_color_type::SPNG_COLOR_TYPE_INDEXED: [[fallthrough]];
      case spng_color_type::SPNG_COLOR_TYPE_TRUECOLOR: [[fallthrough]];
      case spng_color_type::SPNG_COLOR_TYPE_TRUECOLOR_ALPHA:
        format = (ihdr.bit_depth == 16) ? SPNG_FMT_RGBA16 : SPNG_FMT_RGBA8;
        break;
    }

    switch (format) {
      case SPNG_FMT_RGBA8: params.format_ = has_srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; break;
      case SPNG_FMT_RGBA16: params.format_ = VK_FORMAT_R16G16B16A16_UNORM; break;
      case SPNG_FMT_GA8: params.format_ = VK_FORMAT_R8G8_UNORM; break;
      case SPNG_FMT_GA16: params.format_ = VK_FORMAT_R16G16_UNORM; break;
      case SPNG_FMT_G8: params.format_ = VK_FORMAT_R8_UNORM; break;
      default: return false;
    }

    assert(ihdr.width < (1<<16));
    assert(ihdr.height < (1<<16));
    params.size_ = {ihdr.width, ihdr.height};

    *_format = format;
    *_params = params;
    return true;
  }

  bool do_read(image::updator &&_target, spng_ctx &_ctx, uint _rows, spng_format _format) {
    if (spng_decode_image(&_ctx, _target.data(), _target.size_bytes(), _format, SPNG_DECODE_PROGRESSIVE) != SPNG_OK)
      return false;

    auto img_row_pitch = _target.image_row_pitch();
    for (uint row = 0; row < _rows; row++) {
      u8 *target = _target.data() + (row * _target.staging_row_pitch());
      auto errcode = spng_decode_row(&_ctx, target, img_row_pitch);
      if (errcode != SPNG_OK && errcode != SPNG_EOI)
        return false;
    }

    return true;
  }

  std::shared_ptr<image> load_png(display &_display, std::span<const u8> _data) {
    auto ctx = std::unique_ptr<spng_ctx, decltype(&spng_ctx_free)>(spng_ctx_new(0), spng_ctx_free);
    image_params iparams;
    spng_format decode_format;
    if (!prepare_read(&iparams, &decode_format, *ctx, _data))
      return nullptr;

    auto result = std::make_shared<image>(_display, iparams);
    if (!result)
      return nullptr;

    if (!do_read(result->prepare_update(), *ctx, iparams.size_.y, decode_format))
      return nullptr;

    return result;
  }

  shared_subimage load_png(const shared_atlas &_atlas, std::span<const u8> _data) {
    auto ctx = std::unique_ptr<spng_ctx, decltype(&spng_ctx_free)>(spng_ctx_new(0), spng_ctx_free);
    image_params iparams;
    spng_format decode_format;
    if (!prepare_read(&iparams, &decode_format, *ctx, _data))
      return nullptr;

    assert(iparams.format_ == _atlas->image(0)->format());
    auto result = _atlas->alloc(iparams.size_);
    if (!result)
      return nullptr;

    if (!do_read(result->prepare_update(), *ctx, iparams.size_.y, decode_format))
      return nullptr;

    return result;
  }

} // ns hut::imgdec

#endif  // HUT_ENABLE_IMGDEC_LIBSPNG
