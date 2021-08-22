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

#include "hut/atlas_pool.hpp"
#include "hut/image.hpp"
#include "hut/pipeline.hpp"
#include "hut/utils.hpp"

#include "render2d_refl.hpp"

namespace hut::render2d {

  class renderer;
  class quads_set;

  using index = uint32_t;
  struct ubo {
    hut::mat4 proj_ {1};
    hut::mat4 view_ {1};
    float dpi_scale = 1;
  };
  using shared_ubo = hut::shared_ref<ubo>;

  using pipeline = hut::pipeline<index, render2d_vert_spv_refl, render2d_frag_spv_refl,
      const hut::shared_ref<ubo> &, const hut::shared_atlas &, const hut::shared_sampler &>;

  using vertex = pipeline::vertex;
  using instance = pipeline::instance;

  using shared_indices = pipeline::shared_indices;
  using shared_vertices = pipeline::shared_vertices;
  using shared_instances = pipeline::shared_instances;

  enum gradient : u16 {
    T2B, L2R, TL2BR, TR2BL
  };

  // Helper to write to set an instance
  inline void set(instance &_target, u16vec4 _bbox,
                  u8vec4 _from, u8vec4 _to, gradient _gradient = T2B,
                  uint _corner_radius = 0, uint _corner_softness = 0,
                  const shared_subimage &_subimg = shared_subimage{}) {
    _target.col_from_ = _from;
    _target.col_to_ = _to;
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
    }
    else {
      _target.uv_box_ = vec4(0);
    }
    _target.pos_box_.w |= (_gradient & 0xF) << 12;
  }

  class updator {
    friend class quads_set;

    buffer_pool::updator updator_;

    explicit updator(buffer_pool::updator &&_updator) : updator_(std::move(_updator)) {}

  public:
    updator() = delete;
    updator(const updator &) = delete;
    updator& operator=(const updator&) = delete;
    updator(updator &&) = delete;
    updator& operator=(updator&&) = delete;

    instance &operator[](uint _index) {
      u8 *u8ptr = updator_.data() + sizeof(instance) * _index;
      return *reinterpret_cast<instance*>(u8ptr);
    }
  };

  // Holds a contiguous set of instances, upon destruction hide all instances
  class quads_set {
    friend class renderer;

    quads_set(renderer &_parent, uint _batch_index, uint _offset, uint _count)
    : parent_(_parent), batch_index_(_batch_index), offset_(_offset), count_(_count) {}

    renderer &parent_;
    uint batch_index_, offset_, count_;

  public:
    ~quads_set();

    [[nodiscard]] uint count() const { return count_; }

    updator update();
  };

  struct renderer_params : pipeline_params {
    uint initial_count_ = 1024;
  };

  class renderer {
    friend class quads_set;

    struct batch {
      shared_instances buffer_;
      binpack::linear1d<uint> suballocator_;
    };

    std::vector<batch> batches_;

    pipeline pipeline_;
    shared_buffer buffer_;
    shared_atlas atlas_;
    shared_sampler sampler_;
    shared_ubo ubo_;

    void free(quads_set&&);
    batch &grow(uint _count);

  public:
    renderer(render_target &_target, shared_buffer _buffer, shared_atlas _atlas, shared_sampler _sampler, renderer_params _params = {});

    quads_set alloc_uninitialized(uint _count);
    quads_set alloc(std::span<const instance> _initial_data);

    void draw(VkCommandBuffer);

    shared_ubo ubo_ref() { return ubo_; }
  };

} // ns hut::render2d
