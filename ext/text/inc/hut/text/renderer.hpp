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

#define HUT_TEXT_DEBUG_NO_INDIRECT

namespace hut::text {

struct renderer_params : pipeline_params {
  uint initial_mesh_store_size_ = 8 * 1024;
  uint initial_draw_store_size_ = 1024;
};

class renderer {
 public:
  using index_t        = uint16;
  using string_hash    = size_t;
  using glyph_pipeline = pipeline<index_t, glyph_vert_spv_refl, glyph_frag_spv_refl,
                                  const hut::shared_ubo &, const hut::shared_atlas &, const hut::shared_sampler &>;

  using vertex   = glyph_pipeline::vertex;
  using instance = glyph_pipeline::instance;

  using shared_indices   = glyph_pipeline::shared_indices;
  using shared_vertices  = glyph_pipeline::shared_vertices;
  using shared_instances = glyph_pipeline::shared_instances;

  class renderer_suballoc : public suballoc_raw {
    friend class renderer;

    renderer *                     parent_ = nullptr;
    uint                           batch_;
    std::unique_ptr<i16vec4[]>     bboxes_;
    std::unique_ptr<string_hash[]> hashes_;

   public:
    renderer_suballoc()                          = delete;
    renderer_suballoc(const renderer_suballoc &) = delete;
    renderer_suballoc &operator=(const renderer_suballoc &) = delete;

    renderer_suballoc(renderer *_parent, uint _batch, uint _offset_bytes, uint _size_bytes)
        : suballoc_raw(_offset_bytes, _size_bytes)
        , parent_(_parent)
        , batch_(_batch) {}
    renderer_suballoc(renderer_suballoc &&_other) noexcept = default;
    renderer_suballoc &operator=(renderer_suballoc &&_other) noexcept = default;

    ~renderer_suballoc() override {
      if (parent_) renderer_suballoc::release();
    }

    updator  update_raw(uint _offset_bytes, uint _size_bytes) final;
    void     finalize(const updator &_updator) final;
    void     zero_raw(uint _offset_bytes, uint _size_bytes) final;
    VkBuffer underlying_buffer() const final;
    u8 *     existing_mapping() final;
    bool     valid() const final { return parent_ != nullptr; }
    void     release() final;

    i16vec4 bbox(size_t _index) { return bboxes_[_index]; }
    uint    batch() const { return batch_; }
  };
  template<typename T> using suballoc        = suballoc<T, renderer_suballoc>;
  template<typename T> using shared_suballoc = std::shared_ptr<suballoc<T>>;

 public:
  renderer(render_target &_target, shared_buffer _buffer, const shared_font &_font,
           const shared_ubo &_ubo, shared_atlas _atlas, const shared_sampler &_sampler,
           renderer_params _params = renderer_params{});

  shared_suballoc<instance> allocate(std::span<const std::u8string_view> _words);

  void draw(VkCommandBuffer);

 private:
  glyph_pipeline pipeline_;
  shared_buffer  buffer_;
  shared_atlas   atlas_;
  shaper         shaper_;

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
  struct batch;
  struct word {
    uint    alloc_;
    uint    glyphs_;
    uint    ref_count_ = 0;
    i16vec4 bbox_;

    word(renderer *_parent, batch &_batch, uint _alloc, uint _codepoints, std::u8string_view _text);
  };
  struct batch {
    mesh_store mstore_;
    draw_store dstore_;

    std::unordered_map<string_hash, word> cache_;

    batch(batch &&) = default;
    batch(renderer *_parent, uint _mesh_store_size, uint _draw_store_size)
        : mstore_(_parent, _mesh_store_size)
        , dstore_(_parent, _draw_store_size) {}
  };
  std::vector<batch> batches_;

  batch &grow(uint _mesh_store_size, uint _draw_store_size);

  struct words_info {
    std::span<const std::u8string_view> texts_;
    std::unique_ptr<string_hash[]>      hashes_;
    std::unique_ptr<uint[]>             codepoints_;
    uint                                total_codepoints_;

    explicit words_info(std::span<const std::u8string_view> _words);

    [[nodiscard]] uint size() const { return texts_.size(); }
  };
  size_t find_best_fit(const words_info &_info);
};

}  // namespace hut::text
