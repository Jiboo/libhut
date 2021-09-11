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

#include "hut/text/shaper.hpp"

#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>

#include "hut/utils/profiling.hpp"

using namespace hut::text;

shaper::shaper(shared_font _font)
    : buffer_(hb_buffer_create())
    , font_(std::move(_font)) {
}

shaper::~shaper() {
  if (buffer_ != nullptr)
    hb_buffer_destroy(buffer_);
}

void shaper::shape(const shared_atlas &_atlas, std::u8string_view _text, const shape_callback &_cb) {
  HUT_PROFILE_SCOPE_NAMED(PFONT, "shaper::bake {}", ("text"), make_fixed<40>(_text));

  hb_buffer_reset(buffer_);
  hb_buffer_add_utf8(buffer_, (char *)_text.data(), (int)_text.size(), 0, -1);
  hb_buffer_guess_segment_properties(buffer_);
  hb_buffer_set_content_type(buffer_, HB_BUFFER_CONTENT_TYPE_UNICODE);
  hb_shape(font_->font_, buffer_, nullptr, 0);

  uint                 codepoints;
  hb_glyph_info_t     *info = hb_buffer_get_glyph_infos(buffer_, &codepoints);
  hb_glyph_position_t *pos  = hb_buffer_get_glyph_positions(buffer_, nullptr);

  // https://github.com/tangrams/harfbuzz-example/blob/master/src/hbshaper.h
  i16vec2 cur_offset = {0, 0};
  for (uint i = 0; i < codepoints; i++) {
    uint cindex = info[i].codepoint;  // FIXME: Isn't "codepoint" for UTF codes? This seems to be a char index..

    auto it = cache_.find(cindex);
    if (it == cache_.end()) {
      auto result = cache_.emplace(cindex, font_->load(_atlas, cindex));
      assert(result.second);
      it = result.first;
    }
    auto &glyph = it->second;

    if (glyph.subimage_) {
      using i16limits = std::numeric_limits<i16>;
      assert((pos[i].x_offset / font_scale) <= i16limits::max());
      i16vec2 glyph_offset = i16vec2{pos[i].x_offset, pos[i].y_offset} / font_scale;
      i16vec2 offset       = cur_offset + glyph_offset;
      i16vec2 begin{offset.x + glyph.bearing_.x, offset.y - glyph.bearing_.y};
      i16vec2 end = begin + glyph.size_;

      _cb(i, i16vec4{begin, end}, glyph.subimage_->texcoords(), glyph.subimage_->page());
    }
    cur_offset += i16vec2{pos[i].x_advance, pos[i].y_advance} / font_scale;
  }
}
