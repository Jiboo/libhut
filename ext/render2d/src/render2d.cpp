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

using namespace hut;
using namespace hut::render2d;

renderer::renderer(render_target &_target, shared_buffer _buffer,
                   const shared_ubo &_ubo, shared_atlas _atlas, const shared_sampler &_sampler,
                   renderer_params _params)
    : pipeline_(_target, _params)
    , buffer_(std::move(_buffer))
    , atlas_(std::move(_atlas)) {
  if (_params.initial_count_ > 0)
    grow(_params.initial_count_);
  pipeline_.write(0, _ubo, atlas_, _sampler);
}

renderer::batch &renderer::grow(uint _count) {
  return batches_.emplace_back(batch{
      buffer_->allocate<instance>(_count),
      binpack::linear1d<uint>{_count}});
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

renderer::shared_suballoc<instance> renderer::allocate(uint _count, uint _align) {
  auto size_bytes = sizeof(instance) * _count;
  for (uint i = 0; i < batches_.size(); i++) {
    auto fit = batches_[i].suballocator_.pack(size_bytes, _align);
    if (fit)
      return std::make_shared<suballoc<instance>>(this, i, *fit, size_bytes);
  }

  auto &new_buffer = grow(std::max<uint>(size_bytes, batches_.back().size() * 2));
  auto  fit        = new_buffer.suballocator_.pack(size_bytes, _align);
  assert(fit);

  return std::make_shared<suballoc<instance>>(this, uint(batches_.size()) - 1, *fit, size_bytes);
}

suballoc_raw::updator renderer::renderer_suballoc::update_raw(uint _offset_bytes, uint _size_bytes) {
  assert(parent_);
  auto total_offset_bytes = offset_bytes() + _offset_bytes;
  return parent_->batches_[batch_].buffer_->update_raw(total_offset_bytes, _size_bytes);
}

void renderer::renderer_suballoc::finalize(const suballoc_raw::updator &_updator) {
  assert(parent_);
  return parent_->batches_[batch_].buffer_->finalize(_updator);
}

void renderer::renderer_suballoc::zero_raw(uint _offset_bytes, uint _size_bytes) {
  assert(parent_);
  auto total_offset_bytes = offset_bytes() + _offset_bytes;
  return parent_->batches_[batch_].buffer_->zero_raw(total_offset_bytes, _size_bytes);
}

VkBuffer renderer::renderer_suballoc::underlying_buffer() const {
  assert(parent_);
  return parent_->batches_[batch_].buffer_->underlying_buffer();
}

u8 *renderer::renderer_suballoc::existing_mapping() {
  assert(parent_);
  return parent_->batches_[batch_].buffer_->existing_mapping() + offset_bytes();
}

void renderer::renderer_suballoc::release() {
  assert(parent_);
  auto &b = parent_->batches_[batch_];
  b.buffer_->zero_raw(offset_bytes(), size_bytes());
  b.suballocator_.offer(offset_bytes());
  parent_ = nullptr;
}
