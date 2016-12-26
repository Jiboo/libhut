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

buffer::buffer(display &_display, uint32_t _size, VkBufferUsageFlagBits _usage)
    : display_(_display), size_(_size) {
  ranges_.insert(range_t{0, _size, false});

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = _size;
  bufferInfo.usage = _usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(_display.device_, &bufferInfo, nullptr, &buffer_) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create vertex buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(_display.device_, buffer_, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = _display.find_memory_type(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (vkAllocateMemory(_display.device_, &allocInfo, nullptr, &memory_) !=
      VK_SUCCESS) {
    vkDestroyBuffer(_display.device_, buffer_, nullptr);
    throw std::runtime_error("failed to allocate vertex buffer memory!");
  }
  vkBindBufferMemory(display_.device_, buffer_, memory_, 0);
}

buffer::~buffer() {
  vkFreeMemory(display_.device_, memory_, nullptr);
  vkDestroyBuffer(display_.device_, buffer_, nullptr);
}

void buffer::update(uint32_t _offset, uint32_t _size, const void *_data) {
  void *target;
  vkMapMemory(display_.device_, memory_, _offset, _size, 0, &target);
  memcpy(target, _data, _size);
  vkUnmapMemory(display_.device_, memory_);
}

buffer::range_t buffer::do_alloc(uint32_t _size) {
  auto it = ranges_.cbegin();
  for (; it != ranges_.cend(); it++) {
    if (!it->allocated_ && it->size_ >= _size) break;
  }

  if (it == ranges_.cend())
    throw std::out_of_range("couldn't find suitable range");
  // FIXME JB: resize

  range_t result;
  result.offset_ = it->offset_;
  result.size_ = _size;
  result.allocated_ = true;

  range_t free_block = *it;
  ranges_.erase(it);

  free_block.offset_ += _size;
  free_block.size_ -= _size;

  ranges_.insert(result);
  ranges_.insert(free_block);

  return result;
}

void buffer::do_free(uint32_t _offset, uint32_t _size) {
  std::set<range_t>::iterator it = ranges_.find(range_t{_offset, _size, true});
  if (it == ranges_.end())
    throw std::out_of_range("couldn't find range, probably already deleted");

  range_t result = *it;
  result.allocated_ = false;

  it = ranges_.erase(it);
  if (it != ranges_.begin()) {
    // Try to merge with previous node
    it--;
    if (!it->allocated_) {
      result.offset_ = it->offset_;
      result.size_ += it->size_;
      it = ranges_.erase(it);
    } else
      it++;
  }
  // Try to merge with next nodes
  while (!it->allocated_) {
    assert(it->offset_ == result.offset_ + result.size_);
    result.size_ += it->size_;
    it = ranges_.erase(it);
  }

  ranges_.insert(result);
}

void buffer::debug_ranges() {
  for (auto &range : ranges_)
    std::cout << "\trange " << range.offset_ << " to "
              << (range.offset_ + range.size_) << " " << range.allocated_
              << std::endl;
}
