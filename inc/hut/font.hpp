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

#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include "image.hpp"

namespace hut {

  class font {
    friend class shaper;
  private:
    FT_Face face_;
    hb_font_t *font_;
    shared_image atlas_;
    uint load_flags_ = FT_LOAD_NO_BITMAP | FT_LOAD_FORCE_AUTOHINT;

  public:
    font(display &_display, const uint8_t *_addr, size_t _size, glm::uvec2 _atlas_size = {512, 512}) {
      FT_New_Memory_Face(_display.ft_library_, _addr, _size, 0, &face_);
      font_ = hb_ft_font_create(face_, nullptr);
      hb_ft_font_set_load_flags(font_, load_flags_);
      atlas_ = std::make_shared<image>(_display, _atlas_size, VK_FORMAT_R8_UNORM);
      init_binpack();
    }

    ~font() {
      hb_font_destroy (font_);
      FT_Done_Face(face_);
    }

    shared_image atlas() {
      return atlas_;
    }

  private:
    // http://blackpawn.com/texts/lightmaps/default.html
    struct node {
      node* children_[2] = {nullptr, nullptr};
      glm::uvec4 coords_;

      ~node() {
        for (int i = 0; i < 2; i++) {
          if (children_[i] != nullptr)
            delete children_[i];
        }
      }

      bool leaf() { return children_[0] == nullptr && children_[1] == nullptr; }
    };

    struct glyph {
      node *store_ = nullptr;
      glm::vec4 texcoords_ = {0, 0, 0, 0};
      glm::vec2 bearing_ = {0, 0};
      glm::uvec2 bounds_ = {0, 0};

      operator bool() {
        return bounds_.x > 0 && bounds_.y > 0;
      }
    };

    node root_;
    using cache_t = std::unordered_map<uint32_t, glyph>;
    std::unordered_map<uint8_t, cache_t> caches_;

    void init_binpack() {
      root_.coords_ = {0, 0, atlas_->size().x, atlas_->size().y};
    }

    node *binpack(node *_cur_node, const glm::uvec2 &_bounds) {
      if (!_cur_node->leaf()) {
        for (int i = 0; i < 2; i++) {
          node *new_node = binpack(_cur_node->children_[i], _bounds);
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

    glyph &load_glyph(uint _char_index, uint8_t _size) {
      cache_t &cache = caches_[_size];
      auto already_cached = cache.find(_char_index);
      if (already_cached != cache.end())
        return already_cached->second;

      if(FT_Load_Glyph(face_, _char_index, load_flags_))
        throw std::runtime_error("couldn't load char");

      glyph g;
      g.bearing_ = {face_->glyph->bitmap_left, face_->glyph->bitmap_top};

      if (face_->glyph->outline.n_contours > 0) {
        if (FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_NORMAL))
          throw std::runtime_error("couldn't render char");

        FT_Bitmap render = face_->glyph->bitmap;
        g.bounds_ = {render.width, render.rows};
        node *packed = binpack(&root_, g.bounds_);
        if (packed == nullptr) {
          // Can't pack, return same bitmap than for '\0'
          throw std::runtime_error("font altas overflow");
        }
        g.store_ = packed;
        g.texcoords_[0] = float(packed->coords_[0]) / atlas_->size().x;
        g.texcoords_[1] = float(packed->coords_[1]) / atlas_->size().y;
        g.texcoords_[2] = float(packed->coords_[2]) / atlas_->size().x;
        g.texcoords_[3] = float(packed->coords_[3]) / atlas_->size().y;
        atlas_->update(packed->coords_, render.buffer, render.pitch);
      }

      return cache[_char_index] = g;
    }
  };

  using shared_font = std::shared_ptr<font>;

  class shaper {
  private:
    hb_buffer_t *buffer_;

    constexpr static hb_feature_t features[] = {
      { HB_TAG('k', 'e', 'r', 'n'), 1, 0, std::numeric_limits<unsigned int>::max() },
      { HB_TAG('l', 'i', 'g', 'a'), 1, 0, std::numeric_limits<unsigned int>::max() },
      { HB_TAG('l', 'i', 'g', 'a'), 1, 0, std::numeric_limits<unsigned int>::max() }
    };

  public:
    struct result {
      shared_ref<rgba_mask::vertex> vertices_;
      shared_ref<uint16_t> indices_;
      glm::vec4 bbox_;

      operator bool() {
        return vertices_ && indices_;
      }
    };

    shaper() {
      buffer_ = hb_buffer_create();
    }

    ~shaper() {
      hb_buffer_destroy(buffer_);
    }

    glm::vec4 measure(const shared_font &_font, uint8_t _size, const std::string &_text) {
      FT_Set_Char_Size(_font->face_, _size * 64, _size * 64, 96, 96);

    }

    void bake(buffer &_buff, result &_dst, const shared_font &_font, uint8_t _size,
        const std::string &_text, const std::function<void()> _post_cb) {
      FT_Set_Char_Size(_font->face_, _size * 64, _size * 64, 96, 96);

      hb_buffer_reset(buffer_);
      hb_buffer_add_utf8(buffer_, _text.data(), _text.size(), 0, -1);
      hb_buffer_guess_segment_properties(buffer_);

      hb_shape(_font->font_, buffer_, features, sizeof(features) / sizeof(hb_feature_t));

      uint codepoints;
      hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer_, &codepoints);
      hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffer_, nullptr);

      size_t drawn_codepoints = 0;
      size_t max_indices = 6 * codepoints;
      size_t max_vertices = 4 * codepoints;
      auto *indices = new uint16_t[max_indices];
      auto *vertices = new rgba_mask::vertex[max_vertices];
      float x = 0., y = 0;
      glm::vec4 bbox;

      // https://github.com/tangrams/harfbuzz-example/blob/master/src/hbshaper.h
      for (uint i = 0; i < codepoints; i++) {
        uint cindex = info[i].codepoint; // FIXME: Isn't "codepoint" for UTF codes? This seems to be a char index..
        font::glyph &glyph = _font->load_glyph(cindex, _size);

        if (glyph) {
          float xo = ceilf(float(pos[i].x_offset) / 64);
          float yo = ceilf(float(pos[i].y_offset) / 64);

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
          rgba_mask::vertex *vertice = vertices + vertices_offset;
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

        x += floorf(float(pos[i].x_advance) / 64);
        y += floorf(float(pos[i].y_advance) / 64);
      }
      bbox[0] = 0;
      bbox[2] = x;

      _buff.display_.post([_post_cb, &_dst, &_buff, indices, vertices, drawn_codepoints](auto) {
        _dst.indices_ = _buff.allocate<uint16_t>(6 * drawn_codepoints);
        _dst.indices_->set(indices, 6 * drawn_codepoints);
        _dst.vertices_ = _buff.allocate<rgba_mask::vertex>(4 * drawn_codepoints);
        _dst.vertices_->set(vertices, 4 * drawn_codepoints);

        delete [] indices;
        delete [] vertices;
        _post_cb();
      });
    }
  };

}  // namespace hut
