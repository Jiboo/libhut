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
  uint offset = display_.staging_->do_alloc(_size, 4);
  VkBufferCopy copy;
  copy.size = _size;
  copy.srcOffset = offset;
  copy.dstOffset = _offset;

  std::lock_guard lk(display_.staging_mutex_);
  display_.stage_pending_++;
  void *target;
  vkMapMemory(display_.device_, display_.staging_->memory_, offset, _size, 0, &target);
  memcpy(target, _data, _size);
  vkUnmapMemory(display_.device_, display_.staging_->memory_);
  display_.preflush([this, copy](){
    display_.stage_copy(buffer_, &copy);
    display_.stage_pending_--;
  });
}

void buffer::copy_from(VkBuffer _other, uint32_t _other_offset, uint32_t _this_offset, uint32_t _size) {
  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = _other_offset;
  copyRegion.dstOffset = _this_offset;
  copyRegion.size = _size;

  display_.stage_pending_++;
  display_.preflush([this, _other, copyRegion](){
    display_.stage_copy(_other, buffer_, &copyRegion);
    display_.stage_pending_--;
  });
}

void buffer::copy_from(buffer &_other, uint32_t _other_offset, uint32_t _this_offset, uint32_t _size) {
  copy_from(_other.buffer_, _other_offset, _this_offset, _size);
}

void buffer::grow(uint32_t new_size) {
  std::lock_guard lk(ranges_mutex_);
  assert(new_size > size_);

  VkBuffer old_buff = buffer_;
  VkDeviceMemory old_mem = memory_;

  init(new_size, type_, usage_);
  copy_from(old_buff, 0, 0, size_);

  {
    std::lock_guard lk(display_.staging_mutex_);
    display_.postflush([this, old_buff, old_mem] {
      vkFreeMemory(display_.device_, old_mem, nullptr);
      vkDestroyBuffer(display_.device_, old_buff, nullptr);
    });
  }

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
    grow(size_ * 2);
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
