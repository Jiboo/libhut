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

#include <utility>

#include "hut/utils/math.hpp"

#include "hut/atlas.hpp"
#include "hut/image.hpp"
#include "hut/pipeline.hpp"

#include "render2d_refl.hpp"

namespace hut::render2d {

class renderer;
class quads_set;

using index = u32;

using pipeline = hut::pipeline<index, render2d_vert_spv_refl, render2d_frag_spv_refl,
                               const hut::shared_ubo &, const hut::shared_atlas &, const hut::shared_sampler &>;

using vertex   = pipeline::vertex;
using instance = pipeline::instance;

using shared_indices   = pipeline::shared_indices;
using shared_vertices  = pipeline::shared_vertices;
using shared_instances = pipeline::shared_instances;

enum gradient : u16 {
  T2B,
  L2R,
  TL2BR,
  TR2BL
};

// Helper to write to an instance
inline void set(instance &_target, u16vec4 _bbox,
                u8vec4 _from, u8vec4 _to, gradient _gradient = T2B,
                uint _corner_radius = 0, uint _corner_softness = 0,
                const shared_subimage &_subimg = shared_subimage{}) {
  _target.col_from_ = _from;
  _target.col_to_   = _to;
  assert(_bbox.x < 4096);
  assert(_bbox.y < 4096);
  assert(_bbox.z < 4096);
  assert(_bbox.w < 4096);
  _target.pos_box_ = _bbox;
  _target.pos_box_.x |= (_corner_radius & 0xF) << 12;
  _target.pos_box_.y |= (_corner_softness & 0xF) << 12;
  if (_subimg) {
    _target.uv_box_ = _subimg->texcoords() * std::numeric_limits<u16>::max();
    assert(_subimg->page() < 4);
    _target.pos_box_.z |= (_subimg->page() & 0xF) << 12;
  } else {
    _target.uv_box_ = vec4(0);
  }
  _target.pos_box_.w |= (_gradient & 0xF) << 12;
}

struct renderer_params : pipeline_params {
  uint initial_count_ = 1024;
};

class renderer {
 public:
  class renderer_suballoc : public suballoc_raw {
    renderer *parent_ = nullptr;
    uint      batch_;

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

    updator  update_raw(uint _offset_bytes, uint _size_bytes) override;
    void     finalize(const updator &_updator) override;
    void     zero_raw(uint _offset_bytes, uint _size_bytes) override;
    VkBuffer underlying_buffer() const override;
    u8 *     existing_mapping() override;
    bool     valid() const override { return parent_ != nullptr; }
    void     release() override;
  };
  template<typename T> using suballoc        = suballoc<T, renderer_suballoc>;
  template<typename T> using shared_suballoc = std::shared_ptr<suballoc<T>>;

 public:
  renderer(render_target &_target, shared_buffer _buffer,
           const shared_ubo &_ubo, shared_atlas _atlas, const shared_sampler &_sampler,
           renderer_params _params = {});

  void draw(VkCommandBuffer);

  shared_suballoc<instance> allocate(uint _count, uint _align = 4);

 private:
  struct batch {
    shared_instances        buffer_;
    binpack::linear1d<uint> suballocator_;

    [[nodiscard]] uint size() const { return suballocator_.capacity(); }
  };
  std::vector<batch> batches_;

  pipeline      pipeline_;
  shared_buffer buffer_;
  shared_atlas  atlas_;

  batch &grow(uint _count);
};

}  // namespace hut::render2d
