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

#ifdef HUT_DEBUG_STAGING
#  include <iostream>
#endif

#include "hut/utils/profiling.hpp"

#include "hut/buffer.hpp"
#include "hut/display.hpp"

using namespace hut;

buffer::buffer(display &_display, uint _size, const buffer_params &_params)
    : display_(_display)
    , params_(_params) {
  grow(_size);
}

buffer::page_data::page_data(buffer &_parent, uint _size)
    : parent_(_parent)
    , suballocator_(_size) {
  VkBufferCreateInfo create_info = {};
  create_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.size               = _size;
  create_info.usage              = _parent.params_.usage_;
  create_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

  auto device = _parent.display_.device();
  if (vkCreateBuffer(device, &create_info, nullptr, &buffer_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements requirements;
  HUT_PVK(vkGetBufferMemoryRequirements, device, buffer_, &requirements);

  auto type = _parent.display_.find_memory_type(requirements.memoryTypeBits, _parent.params_.type_);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize       = requirements.size;
  alloc_info.memoryTypeIndex      = type.first;

  auto aresult = HUT_PVK(vkAllocateMemory, device, &alloc_info, nullptr, &memory_);
  if (aresult != VK_SUCCESS) {
    HUT_PVK(vkDestroyBuffer, device, buffer_, nullptr);
    throw std::runtime_error("failed to allocate buffer memory!");
  }
  HUT_PVK(vkBindBufferMemory, device, buffer_, memory_, 0);

  if (parent_.params_.permanent_map_) {
    HUT_PVK(vkMapMemory, device, memory_, 0, _size, 0, reinterpret_cast<void **>(&permanent_map_));
    assert(permanent_map_);
  } else {
    permanent_map_ = nullptr;
  }
}

buffer::page_data::page_data(page_data &&_other) noexcept
    : parent_(_other.parent_)
    , suballocator_(std::move(_other.suballocator_))
    , buffer_(_other.buffer_)
    , memory_(_other.memory_)
    , permanent_map_(_other.permanent_map_) {
  _other.buffer_        = VK_NULL_HANDLE;
  _other.memory_        = VK_NULL_HANDLE;
  _other.permanent_map_ = nullptr;
}

buffer::page_data::~page_data() {
  auto device = parent_.display_.device();

  if (permanent_map_)
    HUT_PVK(vkUnmapMemory, device, memory_);
  if (buffer_)
    HUT_PVK(vkDestroyBuffer, device, buffer_, nullptr);
  if (memory_)
    HUT_PVK(vkFreeMemory, device, memory_, nullptr);
}

shared_suballoc_raw buffer::allocate_raw(uint _size_bytes, uint _align) {
  for (uint i = 0; i < pages_.size(); i++) {
    auto fit = pages_[i].suballocator_.pack(_size_bytes, _align);
    if (fit)
      return std::make_shared<buffer_suballoc>(this, i, *fit, _size_bytes);
  }

  auto &new_buffer = grow(std::max<uint>(_size_bytes, pages_.back().size() * 2));
  auto  fit        = new_buffer.suballocator_.pack(_size_bytes, _align);
  assert(fit);

  return std::make_shared<buffer_suballoc>(this, uint(pages_.size()) - 1, *fit, _size_bytes);
}

suballoc_raw::updator buffer::buffer_suballoc::update_raw(uint _offset_bytes, uint _size_bytes) {
  assert(parent_);
  auto &          display = parent_->display_;
  std::lock_guard lk(display.staging_mutex_);
  auto            staging_suballoc = display.staging_->allocate_raw(_size_bytes);
  std::span<u8>   staging_span{staging_suballoc->existing_mapping(), _size_bytes};
  return {shared_from_this(), std::move(staging_suballoc), staging_span, _offset_bytes};
}

void buffer::buffer_suballoc::finalize(const suballoc_raw::updator &_updator) {
  assert(parent_);
  display::buffer_copy copy = {};
  copy.size                 = _updator.size_bytes();
  copy.srcOffset            = _updator.staging_alloc()->offset_bytes();
  copy.source               = _updator.staging_alloc()->underlying_buffer();
  copy.dstOffset            = _updator.total_offset_bytes();
  copy.destination          = _updator.parent_alloc()->underlying_buffer();

  auto &          dsp = parent_->display_;
  std::lock_guard lk(dsp.staging_mutex_);
  dsp.staging_jobs_++;
  dsp.preflush([&dsp, copy]() {
    dsp.stage_copy(copy);
    dsp.staging_jobs_--;
  });
  dsp.postflush([staging = _updator.staging_alloc()]() {});
}

void buffer::buffer_suballoc::zero_raw(uint _offset_bytes, uint _size_bytes) {
  assert(parent_);
  const uint total_offset_bytes = offset_bytes() + _offset_bytes;

  display::buffer_zero zero = {};
  zero.size                 = _size_bytes;
  zero.offset               = total_offset_bytes;
  zero.destination          = underlying_buffer();

  auto &dsp = parent_->display_;
  dsp.staging_jobs_++;
  dsp.preflush([&dsp, zero]() {
    dsp.stage_zero(zero);
    dsp.staging_jobs_--;
  });
}

VkBuffer buffer::buffer_suballoc::underlying_buffer() const {
  assert(parent_);
  return parent_->pages_[page_].buffer_;
}

u8 *buffer::buffer_suballoc::existing_mapping() {
  assert(parent_);
  auto map = parent_->pages_[page_].permanent_map_;
  return map ? map + offset_bytes() : nullptr;
}

void buffer::buffer_suballoc::release() {
  assert(parent_);
  parent_->pages_[page_].suballocator_.offer(offset_bytes());
  parent_ = nullptr;
}

#ifdef HUT_DEBUG_STAGING
void buffer::debug() {
  std::cout << "[buffer] " << this << " contents:" << std::endl;
  for (const auto &page : pages_) {
    std::cout << "\tVkBuffer " << page.buffer_ << " size:" << page.size() << std::endl;

    page.suballocator_.visit_blocks([](const auto &block) {
      std::cout << "\t\trange " << block.offset_ << " - " << (block.offset_ + block.size_)
                << ": " << block.used_
                << std::endl;
      return true;
    });
  }
}
#endif
