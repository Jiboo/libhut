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

#include "hut/text/font.hpp"

#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>

#include "hut/utils/profiling.hpp"
#include "hut/utils/sstream.hpp"

#include "hut/atlas.hpp"

using namespace hut;
using namespace hut::text;

struct FTLibraryHolder {
  FT_Library library_ = nullptr;
  FTLibraryHolder() {
    HUT_PROFILE_SCOPE(PFONT, "FT_Init_FreeType");
    FT_Init_FreeType(&library_);
  }
  ~FTLibraryHolder() {
    if (library_)
      FT_Done_FreeType(library_);
  }
};

font::font(std::span<const u8> _data, uint _size, bool _hinting) {
  static FTLibraryHolder lib_holder;

  load_flags_ = FT_LOAD_COLOR | (_hinting ? FT_LOAD_FORCE_AUTOHINT : FT_LOAD_NO_HINTING);

  {
    HUT_PROFILE_SCOPE(PFONT, "font::font::ft");
    auto result = FT_New_Memory_Face(lib_holder.library_, _data.data(), FT_Long(_data.size_bytes()), 0, &face_);
    if (result != 0)
      throw std::runtime_error(sstream("couldn't load face: ") << result);
    FT_Set_Char_Size(face_, FT_F26Dot6(_size * font_scale), FT_F26Dot6(_size * font_scale), 0, 0);
  }
  {
    HUT_PROFILE_SCOPE(PFONT, "font::font::hb");
    font_ = hb_ft_font_create(face_, nullptr);
    hb_ft_font_set_funcs(font_);
    hb_ft_font_set_load_flags(font_, load_flags_);
  }
}

font::~font() {
  hb_font_destroy(font_);
  FT_Done_Face(face_);
}

font::glyph font::load(const shared_atlas &_atlas, uint _char_index) {
  if (FT_HAS_COLOR(face_))
    assert(_atlas->format() == VK_FORMAT_B8G8R8A8_UNORM);

  HUT_PROFILE_SCOPE(PFONT, "font::load_glyph");

  std::unique_lock lk{mutex_};

  if (FT_Load_Glyph(face_, _char_index, load_flags_))
    throw std::runtime_error("couldn't load char");

  FT_GlyphSlot &ftg = face_->glyph;
  if (FT_Render_Glyph(ftg, FT_RENDER_MODE_NORMAL))
    throw std::runtime_error("couldn't render char");

  FT_Bitmap render = ftg->bitmap;
  glyph     result;
  result.bearing_ = vec2{ftg->bitmap_left, ftg->bitmap_top};
  result.size_    = vec2{render.width, render.rows};

  if (render.width == 0 || render.rows == 0)
    return result;

  auto atlas_format = _atlas->format();
  if ((render.pixel_mode == FT_PIXEL_MODE_BGRA && atlas_format == VK_FORMAT_B8G8R8A8_UNORM)
      || (render.pixel_mode == FT_PIXEL_MODE_GRAY && atlas_format == VK_FORMAT_R8_UNORM)) {
    // same format as atlas, nothing to do
    auto span        = std::span<const u8>{render.buffer, render.pitch * render.rows};
    result.subimage_ = _atlas->pack(result.size_, span, render.pitch);
  } else if (render.pixel_mode == FT_PIXEL_MODE_GRAY && atlas_format == VK_FORMAT_B8G8R8A8_UNORM) {
    result.subimage_ = _atlas->alloc(result.size_);
    auto  update     = result.subimage_->update();
    auto *src        = render.buffer;
    auto *dst        = update.data();
    for (uint y = 0; y < render.rows; y++) {
      auto *src_row = src + render.width * y;
      auto *dst_row = (u8vec4 *)(dst + update.staging_row_pitch() * y);
      for (uint x = 0; x < render.width; x++) {
        auto  src_pixel = *(src_row + x);
        auto &dst_pixel = *(dst_row + x);
        dst_pixel.x = dst_pixel.y = dst_pixel.z = dst_pixel.w
            = src_pixel;  // NOTE JBL: premultiplied, to be coherent with emoji BGRA which are too
      }
    }
  } else {
    throw std::runtime_error("missmatch between glyph and atlas pixel formats");
  }

  return result;
}

font::glyph font::load(const shared_atlas &_atlas, char32_t _unichar) {
  uint index = FT_Get_Char_Index(face_, _unichar);
  return load(_atlas, index);
}
