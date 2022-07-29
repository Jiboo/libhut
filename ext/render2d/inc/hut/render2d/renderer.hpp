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
#include <utility>

#include "hut/utils/color.hpp"
#include "hut/utils/length.hpp"
#include "hut/utils/math.hpp"

#include "hut/atlas.hpp"
#include "hut/image.hpp"
#include "hut/pipeline.hpp"

#include "render2d_refl.hpp"

namespace hut::render2d {

class renderer;
class batch_updators;

using index = u32;

using pipeline = pipeline<index, render2d_vert_spv_refl, render2d_frag_spv_refl, const shared_ubo &,
                          const shared_atlas &, const shared_sampler &>;

using vertex   = pipeline::vertex;
using instance = pipeline::instance;

using shared_indices   = pipeline::shared_indices;
using shared_vertices  = pipeline::shared_vertices;
using shared_instances = pipeline::shared_instances;

enum gradient : u16 { T2B, L2R, TL2BR, TR2BL };
enum mode : u16 { ROUNDED, BORDER, SHADOW };

// Helper to write to an instance
struct box_params {
  u16bbox_px  bbox_{0_px};
  u8vec4_rgba from_{0};
  u8vec4_rgba to_{0};
  gradient    gradient_        = T2B;
  mode        mode_            = ROUNDED;
  uint        corner_radius_   = 0;
  uint        corner_softness_ = 0;
  subimage   *subimg_          = nullptr;
};

inline void set(instance &_target, box_params _params) {
  _target.col_from_ = _params.from_;
  _target.col_to_   = _params.to_;

  assert(_params.bbox_.x <= 0xFFF_px);
  assert(_params.bbox_.y <= 0xFFF_px);
  assert(_params.bbox_.z <= 0xFFF_px);
  assert(_params.bbox_.w <= 0xFFF_px);
  _target.pos_box_ = _params.bbox_;

  assert(_params.corner_radius_ <= 0xF);
  _target.pos_box_.x |= (_params.corner_radius_ & 0xF) << 12;
  assert(_params.corner_softness_ <= 0xF);
  _target.pos_box_.y |= (_params.corner_softness_ & 0xF) << 12;

  if (_params.subimg_ != nullptr) {
    _target.uv_box_ = packSnorm<u16>(_params.subimg_->texcoords());
    assert(_params.subimg_->page() <= 0xF);
    _target.pos_box_.z |= (_params.subimg_->page() & 0xF) << 12;
  } else {
    _target.uv_box_ = vec4(0);
  }

  assert(_params.gradient_ <= 0x3);
  _target.pos_box_.w |= (_params.gradient_ & 0x3) << 12;
  assert(_params.mode_ <= 0x3);
  _target.pos_box_.w |= (_params.mode_ & 0x3) << 14;
}

namespace details {

struct batch;
using render2d_suballoc = suballoc<instance, batch>;
using render2d_updator  = buffer_updator<instance>;

struct batch {
  shared_instances        buffer_;
  binpack::linear1d<uint> suballocator_;

  batch(shared_instances _buffer, uint _instances_count)
      : buffer_(std::move(_buffer))
      , suballocator_(_instances_count) {}

  void                           release(render2d_suballoc *_suballoc);
  [[nodiscard]] render2d_updator update_raw_impl(uint _offset_bytes, uint _size_bytes);
  void                           zero_raw(uint _offset_bytes, uint _size_bytes);

  template<typename TContained>
  [[nodiscard]] buffer_updator<TContained> update_raw(uint _offset_bytes, uint _size_bytes) {
    return update_raw_impl(_offset_bytes, _size_bytes);
  }

  [[nodiscard]] uint size() const { return suballocator_.capacity(); }
};
}  // namespace details

using boxes_holder = suballoc<instance, details::batch>;

class batch_updators {
  friend class renderer;

  std::unordered_map<details::batch*, buffer_updator<instance>> updators_;

public:
  std::span<instance> locate(const boxes_holder&);
};

struct renderer_params : pipeline_params {
  uint initial_batch_size_instances_ = 1024;
};

class renderer {
 public:
  renderer(render_target &_target, shared_buffer _buffer, const shared_ubo &_ubo, shared_atlas _atlas,
           const shared_sampler &_sampler, renderer_params _params = {});

  void draw(VkCommandBuffer _buffer);

  boxes_holder allocate(uint _count);

  batch_updators update_all();

 private:
  std::list<details::batch> batches_;

  pipeline      pipeline_;
  shared_buffer buffer_;
  shared_atlas  atlas_;

  void grow(uint _count);
};

}  // namespace hut::render2d
