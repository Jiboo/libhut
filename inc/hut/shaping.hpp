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

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include "hut/buffer_pool.hpp"
#include "hut/font.hpp"
#include "hut/utils.hpp"

namespace hut {

template<typename TUVType>
TUVType encode_atlas_page_xy(TUVType _in, uint _atlas_page) {
  switch (_atlas_page) {
    case  0: _in.x *= +1; _in.y *= +1; break;
    case  1: _in.x *= +1; _in.y *= -1; break;
    case  2: _in.x *= -1; _in.y *= +1; break;
    case  3: _in.x *= -1; _in.y *= -1; break;
    default:
      throw std::runtime_error("can't use default_vertex_updater_for_glyph for storing atlas_page, not enough bits");
  }
  return _in;
}

template<typename TVertexType>
struct default_vertex_updater_for_glyph {
  void operator()(void*, vec2 _pos, vec2 _uv, uint _atlas_page, TVertexType &_v) const {
    _v.pos_ = _pos;
    _v.uv_ = encode_atlas_page_xy(_uv, _atlas_page);
  };
};

template<typename TIndexType, typename TVertexType, typename TUpdater = default_vertex_updater_for_glyph<TVertexType>>
class shaper {
 private:
  hb_buffer_t *buffer_;

 public:
  struct result {
    friend class shaper;

    shared_ref<TIndexType> indices_;
    shared_ref<TVertexType> vertices_;
    uint indices_count_ = 0;
    uint vertices_count_ = 0;
    vec4 bbox_ = {};

    inline explicit operator bool() const {
      return indices_ && vertices_;
    }
  };

  shaper(){
    buffer_ = hb_buffer_create();
  }

  ~shaper() {
    hb_buffer_destroy(buffer_);
  }

  struct context {
    shared_buffer &buff_;
    result &dst_;
    const shared_font &font_;
    uint8_t size_ = 0;
    const std::string_view text_;
  };

  bool bake(const context &_ctx, void *_updater_param = nullptr) {
    HUT_PROFILE_SCOPE_NAMED(PFONT, "shaper::bake {}", ("text"), make_fixed<40>(_ctx.text_));
    if (_ctx.text_.empty()) {
      if (_ctx.dst_.indices_count_ > 0) {
        _ctx.dst_.indices_->zero(0, _ctx.dst_.indices_count_);
        _ctx.dst_.indices_count_ = 0;
      }
      if (_ctx.dst_.vertices_count_ > 0) {
        _ctx.dst_.vertices_->zero(0, _ctx.dst_.vertices_count_);
        _ctx.dst_.vertices_count_ = 0;
      }
      return false;
    }

    constexpr size_t font_scale = 64;
    std::lock_guard lk(_ctx.font_->baking_mutex_);
    FT_Set_Char_Size(_ctx.font_->face_, _ctx.size_ * font_scale, _ctx.size_ * font_scale, 0, 0);

    auto &cache = _ctx.font_->load_cache(_ctx.size_);
    hb_buffer_reset(buffer_);
    hb_buffer_add_utf8(buffer_, _ctx.text_.data(), _ctx.text_.size(), 0, -1);
    hb_buffer_guess_segment_properties(buffer_);
    hb_buffer_set_content_type(buffer_, HB_BUFFER_CONTENT_TYPE_UNICODE);
    hb_shape(cache.font_, buffer_, nullptr, 0);

    uint codepoints;
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buffer_, &codepoints);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buffer_, nullptr);

    size_t drawn_codepoints = 0;
    size_t max_indices = 6 * codepoints;
    size_t max_vertices = 4 * codepoints;
    TIndexType indices[max_indices];
    TVertexType vertices[max_vertices];
    float x = 0., y = 0;
    vec4 bbox = {0, 0, 0, 0};

    constexpr TUpdater updater;

    // https://github.com/tangrams/harfbuzz-example/blob/master/src/hbshaper.h
    for (uint i = 0; i < codepoints; i++) {
      uint cindex = info[i].codepoint; // FIXME: Isn't "codepoint" for UTF codes? This seems to be a char index..
      font::glyph &glyph = _ctx.font_->load_glyph(cache.glyphs_, cindex);

      if (glyph) {
        float xo = float(pos[i].x_offset) / font_scale;
        float yo = float(pos[i].y_offset) / font_scale;

        float x0 = x + xo + glyph.bearing_.x;
        float y0 = y + yo + glyph.bearing_.y;
        float x1 = x0 + glyph.bounds_.x;
        float y1 = y0 - glyph.bounds_.y;
        y0 *= -1;
        y1 *= -1;

        if (y0 < bbox[1]) bbox[1] = y0;
        if (y1 > bbox[3]) bbox[3] = y1;

        float s0 = glyph.texcoords_[0];
        float t0 = glyph.texcoords_[1];
        float s1 = glyph.texcoords_[2];
        float t1 = glyph.texcoords_[3];

        size_t vertices_offset = 4 * drawn_codepoints;
        TVertexType *vertice = vertices + vertices_offset;
        auto page = glyph.subimage_->page();
        updater(_updater_param, {x0, y0}, {s0, t0}, page, vertice[0]);
        updater(_updater_param, {x0, y1}, {s0, t1}, page, vertice[1]);
        updater(_updater_param, {x1, y0}, {s1, t0}, page, vertice[2]);
        updater(_updater_param, {x1, y1}, {s1, t1}, page, vertice[3]);

        TIndexType *indice = indices + 6 * drawn_codepoints;
        indice[0] = vertices_offset + 0;
        indice[1] = vertices_offset + 1;
        indice[2] = vertices_offset + 2;
        indice[3] = vertices_offset + 2;
        indice[4] = vertices_offset + 1;
        indice[5] = vertices_offset + 3;

        drawn_codepoints++;
      }

      x += std::round(float(pos[i].x_advance) / font_scale);
      y += std::round(float(pos[i].y_advance) / font_scale);
    }
    bbox[0] = 0;
    bbox[2] = x;
    _ctx.dst_.bbox_ = bbox;

    const int indices_count = 6 * static_cast<int>(drawn_codepoints);
    const int prev_indices_count = _ctx.dst_.indices_ ? static_cast<int>(_ctx.dst_.indices_->size()) : 0;
    const int indices_diff = indices_count - prev_indices_count;
    const int clear_indices = static_cast<int>(_ctx.dst_.indices_count_) - indices_count;
    if (indices_diff > 0) {
      _ctx.dst_.indices_ = _ctx.buff_->template allocate<TIndexType>(indices_count);
      _ctx.dst_.indices_->update_some(0, indices_count, indices);
    }
    else if (indices_count > 0) {
      _ctx.dst_.indices_->update_some(0, indices_count, indices);
    }
    if (indices_diff < 0 && clear_indices > 0) {
      _ctx.dst_.indices_->zero(indices_count, clear_indices);
    }
    _ctx.dst_.indices_count_ = indices_count;

    const int vertices_count = 4 * static_cast<int>(drawn_codepoints);
    const int prev_vertices_count = _ctx.dst_.vertices_ ? static_cast<int>(_ctx.dst_.vertices_->size()) : 0;
    const int vertices_diff = vertices_count - prev_vertices_count;
    const int clear_vertices = static_cast<int>(_ctx.dst_.vertices_count_) - vertices_count;
    if (vertices_diff > 0) {
      _ctx.dst_.vertices_ = _ctx.buff_->template allocate<TVertexType>(vertices_count);
      _ctx.dst_.vertices_->update_some(0, vertices_count, vertices);
    }
    else if (vertices_count > 0) {
      _ctx.dst_.vertices_->update_some(0, vertices_count, vertices);
    }
    if (vertices_diff < 0 && clear_vertices > 0) {
      _ctx.dst_.vertices_->zero(vertices_count, clear_vertices);
    }
    _ctx.dst_.vertices_count_ = vertices_count;

    return indices_diff > 0 || vertices_diff > 0;
  }
};

}  // namespace hut
