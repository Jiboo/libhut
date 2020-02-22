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

#include "hut/font.hpp"

using namespace hut;

constexpr size_t font_scale = 64;

font::font(display &_display, const uint8_t *_addr, size_t _size, uvec2 _atlas_size, bool _hinting) {
  load_flags_ = FT_LOAD_NO_BITMAP | (_hinting ? FT_LOAD_FORCE_AUTOHINT : FT_LOAD_NO_HINTING);
  FT_New_Memory_Face(_display.ft_library_, _addr, _size, 0, &face_);

  atlas_ = std::make_shared<image>(_display, _atlas_size, VK_FORMAT_R8_UNORM);
  root_.coords_ = {0, 0, _atlas_size};
}

font::~font() {
  for (auto & cache : caches_)
    hb_font_destroy(cache.second.font_);
  FT_Done_Face(face_);
}

font::node::~node() {
  for (auto & i : children_) {
    delete i;
  }
}

font::node *font::binpack(node *_cur_node, const uvec2 &_bounds) {
  if (!_cur_node->leaf()) {
    for (auto & i : _cur_node->children_) {
      node *new_node = binpack(i, _bounds);
      if (new_node)
        return new_node;
    }
    return nullptr;
  }

  uint node_w = _cur_node->coords_[2] - _cur_node->coords_[0];
  uint node_h = _cur_node->coords_[3] - _cur_node->coords_[1];
  if (_bounds.x > node_w || _bounds.y > node_h)
    return nullptr;

  uint new_node_w = node_w - _bounds.x;
  uint new_node_h = node_h - _bounds.y;
  node *left = _cur_node->children_[0] = new node;
  node *right = _cur_node->children_[1] = new node;
  if (new_node_w <= new_node_h) {
    left->coords_[0] = _cur_node->coords_[0] + _bounds.x;
    left->coords_[1] = _cur_node->coords_[1];
    left->coords_[2] = left->coords_[0] + new_node_w;
    left->coords_[3] = left->coords_[1] + _bounds.y;

    right->coords_[0] = _cur_node->coords_[0];
    right->coords_[1] = _cur_node->coords_[1] + _bounds.y;
    right->coords_[2] = right->coords_[0] + node_w;
    right->coords_[3] = right->coords_[1] + new_node_h;
  }
  else {
    left->coords_[0] = _cur_node->coords_[0];
    left->coords_[1] = _cur_node->coords_[1] + _bounds.y;
    left->coords_[2] = left->coords_[0] + _bounds.x;
    left->coords_[3] = left->coords_[1] + new_node_h;

    right->coords_[0] = _cur_node->coords_[0] + _bounds.x;
    right->coords_[1] = _cur_node->coords_[1];
    right->coords_[2] = right->coords_[0] + new_node_w;
    right->coords_[3] = right->coords_[1] + node_h;
  }

  _cur_node->coords_[2] = _cur_node->coords_[0] + _bounds.x;
  _cur_node->coords_[3] = _cur_node->coords_[1] + _bounds.y;
  return _cur_node;
}

font::glyph &font::load_glyph(glyph_cache_t &_cache, uint _char_index) {
  glyph &g = _cache[_char_index];
  if (g)
    return g;

  if(FT_Load_Glyph(face_, _char_index, load_flags_))
    throw std::runtime_error("couldn't load char");

  FT_GlyphSlot &ftg = face_->glyph;

  if (ftg->outline.n_contours > 0) {
    if (FT_Render_Glyph(ftg, FT_RENDER_MODE_NORMAL))
      throw std::runtime_error("couldn't render char");

    FT_Bitmap render = ftg->bitmap;
    g.bearing_ = {ftg->bitmap_left, ftg->bitmap_top};
    g.bounds_ = {render.width, render.rows};
    const uvec2 padding = {2, 2};
    node *packed = binpack(&root_, g.bounds_ + padding);
    if (packed == nullptr) {
      // Can't pack, return same bitmap than for '\0'
      throw std::runtime_error("font altas overflow");
    }
    g.texcoords_[0] = float(packed->coords_[0]) / atlas_->size().x;
    g.texcoords_[1] = float(packed->coords_[1]) / atlas_->size().y;
    g.texcoords_[2] = float(packed->coords_[2] - padding.x) / atlas_->size().x;
    g.texcoords_[3] = float(packed->coords_[3] - padding.y) / atlas_->size().y;
    atlas_->update(packed->coords_ - uvec4{0, 0, padding}, render.buffer, render.pitch);
  }

  return g;
}

font::cache &font::load_cache(uint8_t _size) {
  auto &cache = caches_[_size];
  if (cache)
    return cache;

  cache.font_ = hb_ft_font_create(face_, nullptr);
  hb_ft_font_set_load_flags(cache.font_, load_flags_);
  return cache;
}

shaper::shaper() {
  buffer_ = hb_buffer_create();
}

shaper::~shaper() {
  hb_buffer_destroy(buffer_);
}

bool shaper::bake(shared_buffer &_buff, result &_dst, const shared_font &_font, uint8_t _size, const std::string_view &_text) {
  assert(!_text.empty());

  std::lock_guard lk(_font->baking_mutex_);
  FT_Set_Char_Size(_font->face_, _size * font_scale, _size * font_scale, 0, 0);

  auto &cache = _font->load_cache(_size);
  hb_buffer_reset(buffer_);
  hb_buffer_add_utf8(buffer_, _text.data(), _text.size(), 0, -1);
  hb_buffer_guess_segment_properties(buffer_);
  hb_shape(cache.font_, buffer_, nullptr, 0);

  uint codepoints;
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buffer_, &codepoints);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buffer_, nullptr);

  size_t drawn_codepoints = 0;
  size_t max_indices = 6 * codepoints;
  size_t max_vertices = 4 * codepoints;
  uint16_t indices[max_indices];
  tex_mask::vertex vertices[max_vertices];
  float x = 0., y = 0;
  vec4 bbox = {0, 0, 0, 0};

  // https://github.com/tangrams/harfbuzz-example/blob/master/src/hbshaper.h
  for (uint i = 0; i < codepoints; i++) {
    uint cindex = info[i].codepoint; // FIXME: Isn't "codepoint" for UTF codes? This seems to be a char index..
    font::glyph &glyph = _font->load_glyph(cache.glyphs_, cindex);

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
      tex_mask::vertex *vertice = vertices + vertices_offset;
      vertice[0] = {{x0, y0}, {s0, t0}};
      vertice[1] = {{x1, y0}, {s1, t0}};
      vertice[2] = {{x1, y1}, {s1, t1}};
      vertice[3] = {{x0, y1}, {s0, t1}};

      uint16_t *indice = indices + 6 * drawn_codepoints;
      indice[0] = vertices_offset + 0;
      indice[1] = vertices_offset + 1;
      indice[2] = vertices_offset + 2;
      indice[3] = vertices_offset + 0;
      indice[4] = vertices_offset + 2;
      indice[5] = vertices_offset + 3;

      drawn_codepoints++;
    }

    x += float(pos[i].x_advance) / font_scale;
    y += float(pos[i].y_advance) / font_scale;
  }
  bbox[0] = 0;
  bbox[2] = x;
  _dst.bbox_ = bbox;

  const uint indices_count = 6 * drawn_codepoints;
  const uint prev_indices_count = _dst.indices_ ? _dst.indices_->size() : 0;
  const int indices_diff = indices_count - prev_indices_count;
  if (indices_diff > 0) {
    _dst.indices_ = _buff->allocate<uint16_t>(indices_count);
    _dst.indices_->update_some(0, indices_count, indices);
  }
  else if (indices_count > 0) {
    _dst.indices_->update_some(0, indices_count, indices);
  }

  const uint vertices_count = 4 * drawn_codepoints;
  const uint prev_vertices_count = _dst.vertices_ ? _dst.vertices_->size() : 0;
  const int vertices_diff = vertices_count - prev_vertices_count;
  if (vertices_diff > 0) {
    _dst.vertices_ = _buff->allocate<tex_mask::vertex>(vertices_count);
    _dst.vertices_->update_some(0, vertices_count, vertices);
  }
  else if (vertices_count > 0) {
    _dst.vertices_->update_some(0, vertices_count, vertices);
  }

  _dst.indices_count_ = indices_count;
  return indices_diff != 0 || vertices_diff != 0;
}