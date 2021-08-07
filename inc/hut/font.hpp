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

#include <unordered_map>
#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include "hut/atlas_pool.hpp"
#include "hut/buffer_pool.hpp"
#include "hut/image.hpp"
#include "hut/pipeline.hpp"

namespace hut {

class font {
  template<typename TIndexType, typename TVertexType, typename TUpdater> friend class shaper;

 public:
  font(display &_display, const uint8_t *_addr, size_t _size, const shared_atlas &_atlas, bool _hinting = true);
  ~font();

 private:
  struct glyph {
    shared_subimage subimage_;
    vec4 texcoords_ {0, 0, 0, 0};
    vec2 bearing_ {0, 0};
    uvec2 bounds_ {0, 0};

    inline explicit operator bool() const { return bounds_.x > 0 && bounds_.y > 0; }
  };

  using glyph_cache_t = std::unordered_map<uint , glyph>;
  struct cache {
    glyph_cache_t glyphs_;
    hb_font_t *font_ = nullptr;

    explicit operator bool() const { return font_ != nullptr; }
  };

  FT_Face face_;
  FT_Library library_;
  FT_Int32 load_flags_;
  shared_atlas atlas_;
  std::unordered_map<uint8_t, cache> caches_;
  std::mutex baking_mutex_;

  glyph &load_glyph(glyph_cache_t &_cache, uint _char_index);
  cache &load_cache(uint8_t _size);
};

using shared_font = std::shared_ptr<font>;

}  // namespace hut
