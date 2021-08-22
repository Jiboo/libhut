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

#include "hut/render2d/render2d.hpp"

#include <utility>

namespace hut::render2d {

  quads_set::~quads_set() {
    parent_.free(std::move(*this));
  }

  updator quads_set::update() {
    uint offset_bytes = sizeof(instance) * offset_;
    uint size_bytes = sizeof(instance) * count_;

    auto underlying = parent_.batches_[batch_index_].buffer_->update_raw_indirect(offset_bytes, size_bytes);
    return updator{std::move(underlying)};
  }

  void renderer::free(quads_set &&_set) {
    batches_[_set.batch_index_].suballocator_.offer(_set.offset_);
  }

  renderer::renderer(render_target &_target,
                     shared_buffer _buffer, shared_atlas _atlas, shared_sampler _sampler,
                     renderer_params _params)
     : pipeline_(_target, _params)
     , buffer_(std::move(_buffer))
     , atlas_(std::move(_atlas))
     , sampler_(std::move(_sampler)) {

    ubo_ = buffer_->allocate<ubo>(1);
    ubo default_ubo;
    const auto target_size = vec2(bbox_size(_target.render_target_params_.box_));
    default_ubo.proj_ = ortho<float>(0.f, target_size.x, 0.f, target_size.y);
    ubo_->update_one(0, default_ubo);

    grow(_params.initial_count_);

    pipeline_.write(0, ubo_, atlas_, sampler_);
  }

  quads_set renderer::alloc_uninitialized(uint _count) {
    for (uint i = 0; i < batches_.size(); i++) {
      auto &batch = batches_[i];
      auto result = batch.suballocator_.pack(_count);
      if (result)
        return quads_set{*this, i, *result, _count};
    }
    auto &new_batch = grow(std::max(_count, batches_[0].buffer_->size()));
    auto result = new_batch.suballocator_.pack(_count);
    assert(result);
    return quads_set{*this, uint(batches_.size()) - 1, *result, _count};
  }

  quads_set renderer::alloc(std::span<const instance> _initial_data) {
    quads_set new_set = alloc_uninitialized(_initial_data.size());
    updator up = new_set.update();
    for (uint i = 0; i < _initial_data.size(); i++)
      up[i] = _initial_data[i];
    return new_set;
  }

  renderer::batch &renderer::grow(uint _count) {
    return batches_.emplace_back(
        buffer_->allocate<instance>(_count),
        binpack::linear1d<uint>{_count});
  }

  void renderer::draw(VkCommandBuffer _buffer) {
    pipeline_.update_atlas(0, atlas_);
    pipeline_.bind_pipeline(_buffer);
    for (const auto &batch : batches_) {
      auto instances_count = batch.suballocator_.upper_bound();
      if (!instances_count)
        continue;
      pipeline_.bind_instances(_buffer, batch.buffer_, 0, instances_count);
      pipeline_.bind_descriptor(_buffer, 0);
      pipeline_.draw(_buffer, 6, instances_count, 0, 0);
    }
  }

} // ns hut::render2d
