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

using index = u32;

using pipeline = hut::pipeline<index, render2d_vert_spv_refl, render2d_frag_spv_refl, const hut::shared_ubo &,
                               const hut::shared_atlas &, const hut::shared_sampler &>;

using vertex   = pipeline::vertex;
using instance = pipeline::instance;

using shared_indices   = pipeline::shared_indices;
using shared_vertices  = pipeline::shared_vertices;
using shared_instances = pipeline::shared_instances;

enum gradient : u16 { T2B, L2R, TL2BR, TR2BL };

// Helper to write to an instance
inline void set(instance &_target, u16vec4 _bbox, u8vec4 _from, u8vec4 _to, gradient _gradient = T2B,
                uint _corner_radius = 0, uint _corner_softness = 0,
                const shared_subimage &_subimg = shared_subimage{}) {
  _target.col_from_ = _from;
  _target.col_to_   = _to;

  assert(_bbox.x <= 0xFFF);
  assert(_bbox.y <= 0xFFF);
  assert(_bbox.z <= 0xFFF);
  assert(_bbox.w <= 0xFFF);
  _target.pos_box_ = _bbox;

  assert(_corner_radius <= 0xF);
  _target.pos_box_.x |= (_corner_radius & 0xF) << 12;
  assert(_corner_softness <= 0xF);
  _target.pos_box_.y |= (_corner_softness & 0xF) << 12;

  if (_subimg) {
    _target.uv_box_ = hut::packSnorm<u16>(_subimg->texcoords());
    assert(_subimg->page() <= 0xF);
    _target.pos_box_.z |= (_subimg->page() & 0xF) << 12;
  } else {
    _target.uv_box_ = vec4(0);
  }

  assert(_gradient <= 0xF);
  _target.pos_box_.w |= (_gradient & 0xF) << 12;
}

namespace details {
struct batch;
using render2d_suballoc = suballoc<instance, batch>;
using render2d_updator  = buffer_updator<instance>;

struct batch {
  shared_instances        buffer_;
  binpack::linear1d<uint> suballocator_;

  void                           release(render2d_suballoc *);
  [[nodiscard]] render2d_updator update_raw_impl(uint _offset_bytes, uint _size_bytes);
  void                           zero_raw(uint _offset_bytes, uint _size_bytes);

  template<typename TContained>
  [[nodiscard]] buffer_updator<TContained> update_raw(uint _offset_bytes, uint _size_bytes) {
    return update_raw_impl(_offset_bytes, _size_bytes);
  }

  [[nodiscard]] uint size() const { return suballocator_.capacity(); }
};
}  // namespace details

struct renderer_params : pipeline_params {
  uint initial_batch_size_bytes_ = 256 * sizeof(instance);
};

class renderer {
 public:
  renderer(render_target &_target, shared_buffer _buffer, const shared_ubo &_ubo, shared_atlas _atlas,
           const shared_sampler &_sampler, renderer_params _params = {});

  void draw(VkCommandBuffer);

  suballoc<instance, details::batch> allocate(uint _count);

 private:
  std::list<details::batch> batches_;

  pipeline      pipeline_;
  shared_buffer buffer_;
  shared_atlas  atlas_;

  details::batch &grow(uint _count);
};

}  // namespace hut::render2d
