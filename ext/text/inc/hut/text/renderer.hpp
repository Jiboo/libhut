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

#include "hut/text/font.hpp"
#include "hut/text/shaper.hpp"

#include "text_refl.hpp"

//#define HUT_TEXT_DEBUG_NO_INDIRECT

namespace hut::text {

class renderer;

using index_t        = uint16;
using string_hash    = size_t;
using glyph_pipeline = pipeline<index_t, glyph_vert_spv_refl, glyph_frag_spv_refl, const hut::shared_ubo &,
                                const hut::shared_atlas &, const hut::shared_sampler &>;

using vertex   = glyph_pipeline::vertex;
using instance = glyph_pipeline::instance;

using shared_indices   = glyph_pipeline::shared_indices;
using shared_vertices  = glyph_pipeline::shared_vertices;
using shared_instances = glyph_pipeline::shared_instances;

namespace details {

struct batch;
class paragraph_holder;

using text_suballoc = suballoc<instance, batch>;
using text_updator  = buffer_updator<instance>;

struct mesh_store {
  shared_vertices         vertices_;
  shared_indices          indices_;
  binpack::linear1d<uint> suballocator_;

  explicit mesh_store(renderer *_parent, uint _size);
};

struct draw_store {
  shared_instances instances_;
#ifndef HUT_TEXT_DEBUG_NO_INDIRECT
  shared_indexed_commands commands_;
#else
  std::vector<VkDrawIndexedIndirectCommand> commands_;
#endif
  binpack::linear1d<uint> suballocator_;

  std::unordered_map<uint, std::vector<string_hash>> hashes_;

  explicit draw_store(renderer *_parent, uint _size);
};

struct word {
  uint    alloc_;
  uint    glyphs_;
  uint    ref_count_ = 0;
  i16vec4 bbox_;

  word(renderer *_parent, batch &_batch, uint _alloc, uint _atlas_page, std::u8string_view _text);
};

struct batch {
  mesh_store mstore_;
  draw_store dstore_;

  std::unordered_map<string_hash, word> cache_;

  batch(renderer *_parent, uint _mesh_store_size, uint _draw_store_size)
      : mstore_(_parent, _mesh_store_size)
      , dstore_(_parent, _draw_store_size) {}

  void                       release_words(std::span<string_hash> _hashes);
  void                       release(text_suballoc *_suballoc);
  [[nodiscard]] text_updator update_raw_impl(uint _offset_bytes, uint _size_bytes);
  void                       zero_raw(uint _offset_bytes, uint _size_bytes);

  template<typename TContained>
  [[nodiscard]] buffer_updator<TContained> update_raw(uint _offset_bytes, uint _size_bytes) {
    return update_raw_impl(_offset_bytes, _size_bytes);
  }
};

class paragraph_holder {
  friend class text::renderer;
  friend class details::batch;

  text_suballoc                  instances_;
  std::unique_ptr<i16vec4[]>     bboxes_;
  std::unique_ptr<string_hash[]> hashes_;

 public:
  paragraph_holder() = delete;
  ~paragraph_holder() { release(); }

  paragraph_holder(const paragraph_holder &) = delete;
  paragraph_holder &operator=(const paragraph_holder &) = delete;

  paragraph_holder(paragraph_holder &&_other) noexcept
      : instances_(std::move(_other.instances_))
      , bboxes_(std::move(_other.bboxes_))
      , hashes_(std::move(_other.hashes_)) {}
  paragraph_holder &operator=(paragraph_holder &&_other) noexcept {
    if (&_other != this) {
      instances_ = std::move(_other.instances_);
      bboxes_    = std::move(_other.bboxes_);
      hashes_    = std::move(_other.hashes_);
    }
    return *this;
  }

  paragraph_holder(batch *_parent, uint _offset_bytes, uint _size_bytes)
      : instances_(_parent, _offset_bytes, _size_bytes) {}

  void release() {
    if (instances_.parent()) {
      instances_.parent()->release_words(std::span{hashes_.get(), size()});
      instances_.release();
    }
  }

  text_suballoc &instances() { return instances_; }
  i16vec4        bbox(size_t _index) { return bboxes_[_index]; }
  uint           size() { return instances_.size(); }
};

}  // namespace details

struct renderer_params : pipeline_params {
  uint initial_mesh_store_size_ = 8 * 1024;
  uint initial_draw_store_size_ = 1024;
};

class renderer {
  friend struct details::mesh_store;
  friend struct details::draw_store;
  friend struct details::word;

 public:
  using paragraph_holder = details::paragraph_holder;

  renderer(render_target &_target, shared_buffer _buffer, const shared_font &_font, const shared_ubo &_ubo,
           shared_atlas _atlas, const shared_sampler &_sampler, renderer_params _params = renderer_params{});

  paragraph_holder allocate(std::span<const std::u8string_view> _words);

  void draw(VkCommandBuffer);

 private:
  glyph_pipeline pipeline_;
  shared_buffer  buffer_;
  shared_atlas   atlas_;
  shaper         shaper_;

  std::list<details::batch> batches_;

  details::batch &grow(uint _mesh_store_size, uint _draw_store_size);

  struct words_info {
    std::span<const std::u8string_view> texts_;
    std::unique_ptr<string_hash[]>      hashes_;
    std::unique_ptr<uint[]>             codepoints_;
    uint                                total_codepoints_;

    explicit words_info(std::span<const std::u8string_view> _words);

    [[nodiscard]] uint size() const { return texts_.size(); }
  };
  details::batch &find_best_fit(const words_info &_info);
};

}  // namespace hut::text
