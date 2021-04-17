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

#include <cstring>
#include <iostream>

#include "hut/buffer_pool.hpp"
#include "hut/display.hpp"
#include "hut/profiling.hpp"

using namespace hut;

buffer_pool::buffer_pool(display &_display, uint _size, VkMemoryPropertyFlags _type, uint _usage)
    : display_(_display), type_(_type), usage_(_usage) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::buffer_pool");
  buffers_.reserve(16);
  grow(_size);
}

buffer_pool::~buffer_pool() {
  for (auto &buffer : buffers_)
    free_buffer(buffer);
}

void buffer_pool::do_update(buffer &_buf, uint _offset, uint _size, const void *_data) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::do_update {}", _size);
  auto update = prepare_update(_buf, _offset, _size);
  memcpy(update.data(), _data, _size);
}

buffer_pool::updator buffer_pool::prepare_update(buffer &_buf, uint _offset, uint _size) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::prepare_update {}", _size);
  std::lock_guard lk(display_.staging_mutex_);
  auto staging = display_.staging_->do_alloc(_size, 4);
  return updator(_buf, staging, std::span<uint8_t>{staging.buffer_->permanent_map_ + staging.offset_, _size}, _offset);
}

void buffer_pool::finalize_update(updator &_update) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::finalize_update {}", _update.size_bytes());
  display::buffer_copy copy = {};
  copy.size = _update.data_.size_bytes();
  copy.srcOffset = _update.staging_.offset_;
  copy.dstOffset = _update.offset_;
  copy.source = _update.staging_.buffer_->buffer_;
  copy.destination = _update.target_.buffer_;

  std::lock_guard lk(display_.staging_mutex_);
  display_.staging_jobs_++;
  display_.preflush([this, copy]() {
    display_.stage_copy(copy);
    display_.staging_jobs_--;
  });
  display_.postflush([staging = _update.staging_]() {
    do_free(staging);
  });
}

void buffer_pool::do_zero(buffer &_buf, uint _offset, uint _size) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::do_zero {}", _size);
  display::buffer_zero zero = {};
  zero.size = _size;
  zero.offset = _offset;
  zero.destination = _buf.buffer_;

  display_.staging_jobs_++;
  display_.preflush([this, zero]() {
    display_.stage_zero(zero);
    display_.staging_jobs_--;
  });
}

buffer_pool::buffer *buffer_pool::grow(uint _size) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::grow {} total {}", _size, total_size_);

  assert(buffers_.size() < buffers_.capacity()); // NOTE JBL: If you hit this, you would invalidate all pointers to elements in this array, increase initial buffer size, or fix a potential leak

  buffer result = {*this, _size};
  total_size_ += _size;

  VkBufferCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.size = _size;
  create_info.usage = usage_;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(display_.device_, &create_info, nullptr, &result.buffer_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements requirements;
  HUT_PVK(vkGetBufferMemoryRequirements, display_.device_, result.buffer_, &requirements);

  auto type = display_.find_memory_type(requirements.memoryTypeBits, type_);
  type_ = type.second;

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = requirements.size;
  alloc_info.memoryTypeIndex = type.first;

  auto aresult = HUT_PVK(vkAllocateMemory, display_.device_, &alloc_info, nullptr, &result.memory_);
  if (aresult != VK_SUCCESS) {
    HUT_PVK(vkDestroyBuffer, display_.device_, result.buffer_, nullptr);
    throw std::runtime_error("failed to allocate buffer memory!");
  }
  HUT_PVK(vkBindBufferMemory, display_.device_, result.buffer_, result.memory_, 0);

  if (type_ == staging_type && usage_ == staging_usage) {
    HUT_PVK(vkMapMemory, display_.device_, result.memory_, 0, _size, 0, reinterpret_cast<void**>(&result.permanent_map_));
    assert(result.permanent_map_);
  }
  else {
    result.permanent_map_ = nullptr;
  }

  return &buffers_.emplace_back(result);
}

buffer_pool::alloc buffer_pool::do_alloc(uint _size, uint _align) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::do_alloc {}", _size);

  for (auto &buffer : buffers_) {
    auto fit = buffer.suballocator_.pack(_size, _align);
    if (fit)
      return alloc{&buffer, *fit, _size};
  }

  auto *new_buffer = grow(align(total_size_ + _size, total_size_));
  auto fit = new_buffer->suballocator_.pack(_size, _align);
  assert(fit);

  return alloc{ new_buffer, *fit, _size };
}

void buffer_pool::do_free(const alloc &_alloc) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::do_free {}", _alloc.size_);
  _alloc.buffer_->suballocator_.offer(_alloc.offset_);
}

void buffer_pool::clear_ranges() {
  for (auto &buffer : buffers_) {
    buffer.suballocator_.reset();
  }
}

void buffer_pool::free_buffer(buffer_pool::buffer &_buff) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::free_buffer");
  if (_buff.permanent_map_)
    HUT_PVK(vkUnmapMemory, display_.device_, _buff.memory_);
  if (_buff.buffer_)
    HUT_PVK(vkDestroyBuffer, display_.device_, _buff.buffer_, nullptr);
  if (_buff.memory_)
    HUT_PVK(vkFreeMemory, display_.device_, _buff.memory_, nullptr);
}

void buffer_pool::debug() {
  std::cout << "[buffer] " << this << " contents:" << std::endl;
  for (const auto &buffer : buffers_) {
    std::cout << "\tVkBuffer " << buffer.buffer_ << " size:" << buffer.size_ << std::endl;

    for (const auto &block : buffer.suballocator_.blocks_) {
      std::cout << "\t\trange " << block.offset_ << " - " << (block.offset_ + block.size_)
                << ": " << block.used_
                << std::endl;
    }
  }
}
