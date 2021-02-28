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

#include <iostream>

#include "hut/font.hpp"
#include "hut/profiling.hpp"

#include FT_BITMAP_H

using namespace hut;

font::font(display &_display, const uint8_t *_addr, size_t _size, const shared_atlas &_atlas, bool _hinting)
  : library_(_display.ft_library_), atlas_(_atlas) {
  HUT_PROFILE_SCOPE(PFONT, "font::font");
  load_flags_ = FT_LOAD_COLOR | (_hinting ? FT_LOAD_FORCE_AUTOHINT : FT_LOAD_NO_HINTING);
  assert(FT_New_Memory_Face(library_, _addr, _size, 0, &face_) == 0);
  if (FT_HAS_COLOR(face_))
    assert(_atlas->params_.format_ == VK_FORMAT_B8G8R8A8_UNORM);
}

font::~font() {
  for (auto &cache : caches_)
    hb_font_destroy(cache.second.font_);
  FT_Done_Face(face_);
}

font::glyph &font::load_glyph(glyph_cache_t &_cache, uint _char_index) {
  glyph &g = _cache[_char_index];
  if (g)
    return g;

  HUT_PROFILE_SCOPE(PFONT, "font::load_glyph");
  if(FT_Load_Glyph(face_, _char_index, load_flags_))
    throw std::runtime_error("couldn't load char");

  FT_GlyphSlot &ftg = face_->glyph;

  if (FT_Render_Glyph(ftg, FT_RENDER_MODE_NORMAL))
    throw std::runtime_error("couldn't render char");

  FT_Bitmap render = ftg->bitmap;
  g.bearing_ = {ftg->bitmap_left, ftg->bitmap_top};
  g.bounds_ = {render.width, render.rows};
  if (g.bounds_.x == 0 && g.bounds_.y == 0)
      return g;

  auto atlas_format = atlas_->params_.format_;
  if ((render.pixel_mode == FT_PIXEL_MODE_BGRA && atlas_format == VK_FORMAT_B8G8R8A8_UNORM)
    || (render.pixel_mode == FT_PIXEL_MODE_GRAY && atlas_format == VK_FORMAT_R8_UNORM)) {
    // same format as atlas, nothing to do
    g.subimage_ = atlas_->pack(g.bounds_, render.buffer, render.pitch);
    g.texcoords_ = g.subimage_->texcoords();
  }
  else if (render.pixel_mode == FT_PIXEL_MODE_GRAY && atlas_format == VK_FORMAT_B8G8R8A8_UNORM) {
    FT_Bitmap converted = render;
    converted.pixel_mode = FT_PIXEL_MODE_BGRA;
    FT_Bitmap_Init(&converted);
    FT_Bitmap_Convert(library_, &render, &converted, 4);
    g.subimage_ = atlas_->pack(g.bounds_, converted.buffer, converted.pitch);
    g.texcoords_ = g.subimage_->texcoords();
    FT_Bitmap_Done(library_, &converted);
  }
  else {
    throw std::runtime_error("unexpected glyph format");
  }

  return g;
}

font::cache &font::load_cache(uint8_t _size) {
  auto &cache = caches_[_size];
  if (cache)
    return cache;

  cache.font_ = hb_ft_font_create(face_, nullptr);
  hb_ft_font_set_funcs(cache.font_);
  hb_ft_font_set_load_flags(cache.font_, load_flags_);
  return cache;
}

