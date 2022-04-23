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

namespace hut {

buffer::buffer(display &_display, uint _size, const buffer_params &_params)
    : display_(_display)
    , params_(_params) {
  grow(_size);
}

buffer_suballoc<u8> buffer::allocate_raw(uint _size_bytes, uint _align) {
  HUT_PROFILE_FUN(PBUFFER, _size_bytes, _align)
  for (auto &page : pages_) {
    auto fit = page.suballocator_.pack(_size_bytes, _align);
    if (fit)
      return {&page, *fit, _size_bytes};
  }

  auto &new_buffer = grow(std::max<uint>(_size_bytes, pages_.back().size() * 2));
  auto  fit        = new_buffer.suballocator_.pack(_size_bytes, _align);
  assert(fit);

  return {&new_buffer, *fit, _size_bytes};
}

namespace details {

buffer_page_data::buffer_page_data(buffer &_parent, uint _size)
    : parent_(&_parent)
    , suballocator_(_size) {
  HUT_PROFILE_FUN(PBUFFER, _size)
  VkBufferCreateInfo create_info = {};
  create_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.size               = _size;
  create_info.usage              = _parent.params_.usage_;
  create_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

  auto *device = _parent.display_.device();
  if (HUT_PVK(vkCreateBuffer, device, &create_info, nullptr, &buffer_) != VK_SUCCESS) {
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

  if (parent_->params_.permanent_map_) {
    HUT_PVK(vkMapMemory, device, memory_, 0, _size, 0, reinterpret_cast<void **>(&permanent_map_));
    assert(permanent_map_);
  } else {
    permanent_map_ = nullptr;
  }
}

buffer_page_data::~buffer_page_data() {
  if (parent_ != nullptr) {
    HUT_PROFILE_FUN(PBUFFER)
    auto *device = parent_->display_.device();

    if (permanent_map_ != nullptr)
      HUT_PVK(vkUnmapMemory, device, memory_);
    if (buffer_ != nullptr)
      HUT_PVK(vkDestroyBuffer, device, buffer_, nullptr);
    if (memory_ != nullptr)
      HUT_PVK(vkFreeMemory, device, memory_, nullptr);
  }
}

void buffer_page_data::release_impl(buffer_suballoc<u8> *_suballoc) {
  HUT_PROFILE_FUN(PBUFFER)
  assert(parent_);
  assert(_suballoc->parent_ == this);
  suballocator_.offer(_suballoc->offset_bytes());
}

buffer_updator<u8> buffer_page_data::update_raw_impl(uint _offset_bytes, uint _size_bytes) {
  HUT_PROFILE_FUN(PBUFFER, _offset_bytes, _size_bytes)
  assert(parent_);
  assert(_offset_bytes + _size_bytes <= size());
  auto         &display = parent_->display_;
  auto          lk      = std::lock_guard{display.staging_mutex_};
  auto          staging = display.staging_->allocate_raw(_size_bytes);
  std::span<u8> staging_span{staging.parent_->permanent_map_ + staging.offset_bytes(), _size_bytes};
  return buffer_updator<u8>{*this, std::move(staging), staging_span, _offset_bytes};
}

void buffer_page_data::finalize_impl(buffer_updator<u8> *_updator) {
  HUT_PROFILE_FUN_NAMED(PBUFFER, ("offset", "size"), _updator->offset_bytes(), _updator->size_bytes())
  assert(parent_);
  assert(_updator->parent_ == this);
  auto &display = parent_->display_;

  auto copy         = display::buffer_copy{};
  copy.size         = _updator->size_bytes();
  copy.srcOffset    = _updator->staging_alloc_.offset_bytes();
  copy.source_      = _updator->staging_alloc_.parent_->buffer_;
  copy.dstOffset    = _updator->offset_bytes();
  copy.destination_ = buffer_;

  std::lock_guard lk(display.staging_mutex_);
  display.staging_jobs_++;
  display.preflush([&display, copy]() {
    display.stage_copy(copy);
    display.staging_jobs_--;
  });
  display.postflush_collect(std::move(_updator->staging_alloc_));
}

void buffer_page_data::zero_raw(uint _offset_bytes, uint _size_bytes) const {
  HUT_PROFILE_FUN(PBUFFER, _offset_bytes, _size_bytes)
  assert(parent_);
  assert(_offset_bytes + _size_bytes <= size());
  display::buffer_zero zero = {};
  zero.size_                = _size_bytes;
  zero.offset_              = _offset_bytes;
  zero.destination_         = buffer_;

  auto &dsp = parent_->display_;
  dsp.staging_jobs_++;
  dsp.preflush([&dsp, zero]() {
    dsp.stage_zero(zero);
    dsp.staging_jobs_--;
  });
}

}  // namespace details

#ifdef HUT_DEBUG_STAGING
void buffer::debug() {
  std::cout << "[hut] buffer " << this << " contents:" << std::endl;
  for (const auto &page : pages_) {
    std::cout << "\tVkBuffer " << page.buffer_ << " size:" << page.size() << std::endl;

    page.suballocator_.visit_blocks([](const auto &_block) {
      std::cout << "\t\trange " << _block.offset_ << " - " << (_block.offset_ + _block.size_) << ": " << _block.used_
                << std::endl;
      return true;
    });
  }
}
#endif

}  // namespace hut
