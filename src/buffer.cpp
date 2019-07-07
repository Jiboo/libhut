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

#include <algorithm>
#include <cstring>
#include <iostream>

#include "hut/buffer.hpp"
#include "hut/display.hpp"

using namespace hut;

buffer::buffer(display &_display, uint32_t _size, VkMemoryPropertyFlags _type, uint _usage)
    : display_(_display), size_(_size), usage_(_usage) {
  ranges_.emplace_back(0, _size, false);
  init(_size, _type, _usage);
}

buffer::~buffer() {
  if (memory_ != 0)
    vkFreeMemory(display_.device_, memory_, nullptr);
  if (buffer_ != 0)
    vkDestroyBuffer(display_.device_, buffer_, nullptr);
}


void buffer::init(uint32_t _size, VkMemoryPropertyFlags _type, uint _usage) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = _size;
  bufferInfo.usage = _usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(display_.device_, &bufferInfo, nullptr, &buffer_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create vertex buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(display_.device_, buffer_, &memRequirements);

  auto type = display_.find_memory_type(memRequirements.memoryTypeBits, _type);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = type.first;
  type_ = type.second;

  if (vkAllocateMemory(display_.device_, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
    vkDestroyBuffer(display_.device_, buffer_, nullptr);
    throw std::runtime_error("failed to allocate vertex buffer memory!");
  }
  vkBindBufferMemory(display_.device_, buffer_, memory_, 0);
}

void buffer::update(uint32_t _offset, uint32_t _size, const void *_data) {
  std::lock_guard lk(display_.staging_mutex_);
  uint offset = display_.staging_->do_alloc(_size, 4);

  void *target;
  vkMapMemory(display_.device_, display_.staging_->memory_, offset, _size, 0, &target);
  memcpy(target, _data, _size);
  vkUnmapMemory(display_.device_, display_.staging_->memory_);
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] buffer " << buffer_ << " update at " << display_.staging_->buffer_
            << '[' << _offset << '-' << (_offset + _size) << ']' << std::endl;
#endif

  display::buffer_copy copy = {};
  copy.size = _size;
  copy.srcOffset = offset;
  copy.dstOffset = _offset;
  copy.source = display_.staging_->buffer_;
  copy.destination = buffer_;

  display_.staging_jobs_++;
  display_.preflush([this, copy]() {
    display_.stage_copy(copy);
    display_.staging_jobs_--;
  });
}

void buffer::grow(uint32_t new_size) {
  merge();

  std::lock_guard lk(ranges_mutex_);
  assert(new_size > size_);

  // keep thus ref to delete them once we staged the copy from the old buf to the new
  VkBuffer old_buff = buffer_;
  VkDeviceMemory old_mem = memory_;

  init(new_size, type_, usage_);
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] buffer grow " << old_buff << " changes its ID to " << buffer_ << std::endl;
#endif

  // Find the minimal copy size, otherwise ("full range") we could erase stuff already staged in the new buff!
  uint lower_bound = std::numeric_limits<uint>::max();
  uint upper_bound = 0;
  for (const auto &range : ranges_) {
    if (!range.allocated_)
      continue;
    if (range.offset_ < lower_bound)
      lower_bound = range.offset_;
    uint end = range.offset_ + range.size_;
    if (end > upper_bound)
      upper_bound = end;
  }

  display::buffer_copy copy = {};
  copy.srcOffset = lower_bound;
  copy.dstOffset = lower_bound;
  copy.size = upper_bound - lower_bound;
  copy.source = old_buff;
  copy.destination = buffer_;

  // NOTE: We asked for a growth, while locking the staging, like the alloc for loading a png
  // In this case we can't lock the mutex, as it's already done
  // FIXME: Should we check if this thread is really the one locking mutex?
  if (display_.staging_.get() != this && display_.staging_mutex_.try_lock())
    std::lock_guard lk_staging(display_.staging_mutex_);

  display_.staging_jobs_++;
  display_.preflush([this, copy]() {
#ifdef HUT_DEBUG_STAGING
    std::cout << "[staging] preflush copying old buff " << copy.source << " to " << copy.destination << std::endl;
#endif
    display_.stage_copy(copy);
    display_.staging_jobs_--;
  });
  display_.postflush([this, old_buff, old_mem] {
#ifdef HUT_DEBUG_STAGING
    std::cout << "[staging] destroying old buff " << old_buff << std::endl;
#endif
    vkFreeMemory(display_.device_, old_mem, nullptr);
    vkDestroyBuffer(display_.device_, old_buff, nullptr);
  });

  auto last = ranges_.rbegin();
  if (last->allocated_) {
    range_t new_range;
    new_range.allocated_ = false;
    new_range.size_ = (new_size - size_);
    new_range.offset_ = last->offset_ + last->size_;
    ranges_.push_back(new_range);
  } else {
    range_t &tail = *last;
    tail.size_ += (new_size - size_);
  }

  size_ = new_size;
}

uint buffer::do_alloc(uint32_t _size, uint8_t _align) {
  ranges_mutex_.lock();

  assert(!ranges_.empty());

  uint aligned, aligned_size, align_bytes;
  auto it = ranges_.begin();
  for (; it != ranges_.end(); it++) {
    if (it->allocated_)
      continue;
    aligned = align<uint32_t>(it->offset_, _align);
    align_bytes = aligned - it->offset_;
    aligned_size = align_bytes + _size;
    if (aligned_size > it->size_)
      continue;
    break;
  }

  if (it == ranges_.cend()) {
    ranges_mutex_.unlock();
    grow(align(size_ + _size, size_));
    return do_alloc(_size, _align);
  }

  range_t &free_block = *it;

  if (free_block.size_ - aligned_size == 0) {
    it->allocated_ = true;
  }
  else {
    ranges_.emplace(it, it->offset_, aligned_size, true);
    free_block.size_ -= aligned_size;
    free_block.offset_ += aligned_size;
  }

  ranges_mutex_.unlock();

  assert(aligned % _align == 0);
  return aligned;
}

void buffer::do_free(uint32_t _offset) {
  {
    std::lock_guard lk(ranges_mutex_);
    for (auto &block : ranges_) {
      if (block.offset_ == _offset) {
        block.allocated_ = false;
        break;
      }
    }
  }
  merge();
}

void buffer::debug_ranges() {
  for (auto &range : ranges_)
    std::cout << "\trange " << range.offset_ << " to " << (range.offset_ + range.size_) << " " << range.allocated_
              << std::endl;
}

void buffer::merge() {
  std::lock_guard lk(ranges_mutex_);
  if (ranges_.size() < 2)
    return;

  auto prev = ranges_.begin();
  auto it = std::next(prev);
  while (it != ranges_.end()) {
    if (!it->allocated_ && !prev->allocated_) {
      range_t m = *prev;
      m.size_ += it->size_;
      ranges_.erase(prev);
      it = ranges_.erase(it);
      prev = ranges_.insert(it, m);
      it = prev;
      it++;
    } else {
      it++;
      prev++;
    }
  }
}

void buffer::clear_ranges() {
  std::lock_guard lk(ranges_mutex_);
  ranges_.clear();
  ranges_.emplace_back(0, size_, false);
}
