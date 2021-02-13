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
  shared_ref<u8> staging;
  {
    std::lock_guard lk(display_.staging_mutex_);
    staging = display_.staging_->allocate<u8>(_size, 4);
  }

  memcpy(staging->buff().permanent_map_ + staging->offset_bytes(), _data, _size);

  display::buffer_copy copy = {};
  copy.size = _size;
  copy.srcOffset = staging->offset_bytes();
  copy.dstOffset = _offset;
  copy.source = staging->vkBuffer();
  copy.destination = _buf.buffer_;

  std::lock_guard lk(display_.staging_mutex_);
  display_.staging_jobs_++;
  display_.preflush([this, copy, staging = std::move(staging)]() {
    display_.stage_copy(copy);
    display_.staging_jobs_--;
  });
}

buffer_pool::updator buffer_pool::prepare_update(buffer &_buf, uint _offset, uint _size) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::prepare_update {}", _size);
  std::lock_guard lk(display_.staging_mutex_);
  auto staging = display_.staging_->do_alloc(_size, 4);
  return updator(_buf, staging, span<uint8_t>{staging.buffer_->permanent_map_ + staging.offset_, _size}, _offset);
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
  display_.preflush([this, copy, staging = _update.staging_]() {
    display_.stage_copy(copy);
    do_free(staging.node_);
    display_.staging_jobs_--;
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

  buffer result = {*this};
  result.size_ = _size;
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

  result.root_ = new range;
  result.root_->offset_ = 0;
  result.root_->size_ = _size;
  result.root_->allocated_ = false;
  result.root_->next_ = nullptr;
  result.root_->prev_ = nullptr;

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

#ifdef HUT_DEBUG_ALLOCATIONS
  std::cout << "[alloc] pre-do_alloc" << std::endl;
  debug();
#endif

  uint aligned = 0, aligned_size = _size;
  range *fit = nullptr;
  buffer *container = nullptr;
  for (auto &buffer : buffers_) {
    if (fit != nullptr)
      break;

    range *cur = buffer.root_;
    assert(cur);
    while (cur != nullptr) {
      if (cur->allocated_) {
        cur = cur->next_;
        continue;
      }
      aligned = align<uint>(cur->offset_, _align);
      uint align_bytes = aligned - cur->offset_;
      aligned_size = align_bytes + _size;
      if (aligned_size > cur->size_) {
        cur = cur->next_;
        continue;
      }
      fit = cur;
      container = &buffer;
      break;
    }
  }

  if (fit == nullptr) {
    container = grow(align(total_size_ + _size, total_size_));
    fit = container->root_;
    aligned = 0;
    aligned_size = _size;
  }

  assert(!fit->allocated_);
  assert(fit->size_ >= aligned_size);

  if (fit->size_ == aligned_size) {
    fit->allocated_ = true;
  }
  else {
    auto new_range = new range;
    new_range->size_ = fit->size_ - aligned_size;
    new_range->offset_ = fit->offset_ + aligned_size;
    new_range->allocated_ = false;
    new_range->prev_ = fit;
    new_range->next_ = fit->next_;

    fit->next_ = new_range;
    fit->size_ = aligned_size;
    fit->allocated_ = true;

    if (new_range->next_ != nullptr) {
      new_range->next_->prev_ = new_range;
    }
  }

#ifdef HUT_DEBUG_ALLOCATIONS
  std::cout << "[alloc] post-do_alloc " << fit->offset_ << " - " << (fit->offset_ + fit->size_)
                << ": " << fit->allocated_ << std::endl;
  debug();
#endif

  return alloc{ container, fit, aligned, _size };
}

void buffer_pool::do_free(range *_range) {
  HUT_PROFILE_SCOPE(PBUFFER, "buffer_pool::do_free {}", _range->size_);

#ifdef HUT_DEBUG_ALLOCATIONS
  std::cout << "[alloc] pre-do_free" << _range->offset_ << " - " << (_range->offset_ + _range->size_)
                                     << ": " << _range->allocated_ << std::endl;
  debug();
#endif

  range *prev = _range->prev_, *next = _range->next_;
  const bool prev_free = prev != nullptr && !prev->allocated_;
  const bool next_free = next != nullptr && !next->allocated_;
  if (prev_free && next_free) {
    // merges _range and next into prev
    prev->size_ += _range->size_ + next->size_;
    prev->next_ = next->next_;
    if (next->next_)
      next->next_->prev_ = prev;
    delete _range;
    delete next;
  }
  else if (prev_free) {
    // merge _range into prev
    prev->size_ += _range->size_;
    prev->next_ = next;
    if (next->next_)
      next->prev_ = prev;
    delete _range;
  }
  else if (next_free) {
    // merge next into _range
    _range->size_ += next->size_;
    _range->allocated_ = false;
    _range->next_ = next->next_;
    if (next->next_)
      next->next_->prev_ = _range;
    delete next;
  }
  else {
    _range->allocated_ = false;
  }

#ifdef HUT_DEBUG_ALLOCATIONS
  std::cout << "[alloc] post-do_free" << std::endl;
  debug();
#endif
}

void buffer_pool::clear_ranges() {
  for (const auto &buffer : buffers_) {
    range *cur = buffer.root_;
    cur = cur->next_;
    while(cur != nullptr) {
      range *next = cur->next_;
      delete cur;
      cur = next;
    }
    cur = buffer.root_;
    cur->next_ = nullptr;
    cur->size_ = buffer.size_;
    cur->allocated_ = false;
    assert(cur->offset_ == 0);
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

  if (_buff.root_) {
    range *cur = _buff.root_;
    while (cur != nullptr) {
      range *next = cur->next_;
      delete cur;
      cur = next;
    }
  }
}

void buffer_pool::debug() {
  std::cout << "[buffer] " << this << " contents:" << std::endl;
  for (const auto &buffer : buffers_) {
    std::cout << "\tVkBuffer " << buffer.buffer_ << " size:" << buffer.size_ << std::endl;

    range *cur = buffer.root_;
    while (cur != nullptr) {
      std::cout << "\t\trange " << cur << " " << cur->offset_ << " - " << (cur->offset_ + cur->size_)
                << ": " << cur->allocated_
                << " (p: " << cur->prev_ << ", n: " << cur->next_ << ")"
                << std::endl;
      cur = cur->next_;
    }
  }
}
