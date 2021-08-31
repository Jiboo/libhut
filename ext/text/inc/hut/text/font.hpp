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

#include <memory>
#include <unordered_map>

#include "hut/atlas.hpp"
#include "hut/buffer.hpp"
#include "hut/image.hpp"
#include "hut/pipeline.hpp"

struct FT_FaceRec_;
typedef FT_FaceRec_ *FT_Face;
struct hb_font_t;

namespace hut::text {

static constexpr size_t font_scale = 64;

class font {
  friend class shaper;

 public:
  font(std::span<const u8> _data, uint _size, bool _hinting = true);
  ~font();

  struct glyph {
    shared_subimage subimage_;
    i16vec2         bearing_, size_;
  };

  glyph load(const shared_atlas &, uint _char_index);
  glyph load(const shared_atlas &, char32_t _unichar);

 private:
  FT_Face    face_;
  hb_font_t *font_;
  i32        load_flags_;
  std::mutex mutex_;
};
using shared_font = std::shared_ptr<font>;

}  // namespace hut::text
