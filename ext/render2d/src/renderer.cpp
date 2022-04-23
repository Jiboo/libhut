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

#include "hut/render2d/renderer.hpp"

#include <utility>

namespace hut::render2d {

renderer::renderer(render_target &_target, shared_buffer _buffer, const shared_ubo &_ubo, shared_atlas _atlas,
                   const shared_sampler &_sampler, renderer_params _params)
    : pipeline_(_target, _params)
    , buffer_(std::move(_buffer))
    , atlas_(std::move(_atlas)) {
  if (_params.initial_batch_size_instances_ > 0)
    grow(_params.initial_batch_size_instances_);
  pipeline_.write(0, _ubo, atlas_, _sampler);
}

details::batch &renderer::grow(uint _instances_count) {
  details::batch &result = batches_.emplace_back(
      details::batch{buffer_->allocate<instance>(_instances_count), binpack::linear1d<uint>{_instances_count}});
  result.buffer_->zero();
  return result;
}

void renderer::draw(VkCommandBuffer _buffer) {
  pipeline_.update_atlas(0, atlas_);
  pipeline_.bind_pipeline(_buffer);
  pipeline_.bind_descriptor(_buffer, 0);
  for (const auto &batch : batches_) {
    if (batch.suballocator_.empty())
      continue;
    pipeline_.bind_instances(_buffer, batch.buffer_);

    constexpr bool OPTIMIZE = true;
    const uint     lower    = OPTIMIZE ? batch.suballocator_.lower_bound() : 0;
    const uint     upper    = OPTIMIZE ? batch.suballocator_.upper_bound() : batch.suballocator_.capacity();
    assert(upper >= lower);
    pipeline_.draw(_buffer, 6, upper - lower, 0, lower);
  }
}

suballoc<instance, details::batch> renderer::allocate(uint _instances_count) {
  uint size_bytes = _instances_count * sizeof(instance);
  for (auto &batch : batches_) {
    auto fit = batch.suballocator_.pack(_instances_count);
    if (fit)
      return {&batch, uint(*fit * sizeof(instance)), size_bytes};
  }

  const uint back_size = batches_.back().size();
  auto      &new_batch = grow(std::max<uint>(_instances_count, back_size * 2));
  const auto fit       = new_batch.suballocator_.pack(_instances_count);
  assert(fit);

  return {&new_batch, uint(*fit * sizeof(instance)), size_bytes};
}

namespace details {

void batch::release(render2d_suballoc *_suballoc) {
  assert(_suballoc->parent() == this);
  _suballoc->zero();
  suballocator_.offer(_suballoc->offset());
}

render2d_updator batch::update_raw_impl(uint _offset_bytes, uint _size_bytes) {
  return buffer_->update_raw(_offset_bytes, _size_bytes);
}

void batch::zero_raw(uint _offset_bytes, uint _size_bytes) {
  buffer_->zero_raw(_offset_bytes, _size_bytes);
}

}  // namespace details

}  // namespace hut::render2d
