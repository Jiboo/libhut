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
  if (_params.initial_batch_size_bytes_ > 0)
    grow(_params.initial_batch_size_bytes_);
  pipeline_.write(0, _ubo, atlas_, _sampler);
}

details::batch &renderer::grow(uint _count) {
  return batches_.emplace_back(details::batch{buffer_->allocate<instance>(_count), binpack::linear1d<uint>{_count}});
}

void renderer::draw(VkCommandBuffer _buffer) {
  pipeline_.update_atlas(0, atlas_);
  pipeline_.bind_pipeline(_buffer);
  pipeline_.bind_descriptor(_buffer, 0);
  for (const auto &batch : batches_) {
    pipeline_.bind_instances(_buffer, batch.buffer_);
    pipeline_.draw(_buffer, 6, batch.suballocator_.upper_bound(), 0, 0);
  }
}

suballoc<instance, details::batch> renderer::allocate(uint _count) {
  uint size_bytes = sizeof(instance) * _count;
  for (auto &batch : batches_) {
    auto fit = batch.suballocator_.pack(_count);
    if (fit)
      return {&batch, *fit, size_bytes};
  }

  auto &new_batch = grow(std::max<uint>(_count, batches_.back().size() * 2));
  auto  fit       = new_batch.suballocator_.pack(_count);
  assert(fit);

  return {&new_batch, *fit, size_bytes};
}

namespace details {

void batch::release(render2d_suballoc *_suballoc) {
  assert(_suballoc->parent() == this);
  buffer_->zero_raw(_suballoc->offset_bytes(), _suballoc->size_bytes());
  suballocator_.offer(_suballoc->offset_bytes());
}

render2d_updator batch::update_raw_impl(uint _offset_bytes, uint _size_bytes) {
  return buffer_->update_raw(_offset_bytes, _size_bytes);
}

void batch::zero_raw(uint _offset_bytes, uint _size_bytes) {
  buffer_->zero_raw(_offset_bytes, _size_bytes);
}

}  // namespace details

}  // namespace hut::render2d
