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

#pragma once

#include "hut/utils/binpacks.hpp"

#include "hut/display.hpp"
#include "hut/suballoc.hpp"

namespace hut {

namespace details {
struct buffer_page_data {
  buffer                 *parent_ = nullptr;
  binpack::linear1d<uint> suballocator_;
  VkBuffer                buffer_        = VK_NULL_HANDLE;
  VkDeviceMemory          memory_        = VK_NULL_HANDLE;
  u8                     *permanent_map_ = nullptr;

  buffer_page_data() = delete;
  ~buffer_page_data();

  buffer_page_data(const buffer_page_data &)            = delete;
  buffer_page_data &operator=(const buffer_page_data &) = delete;

  buffer_page_data(buffer_page_data &&_other) noexcept
      : parent_(std::exchange(_other.parent_, nullptr))
      , suballocator_(std::move(_other.suballocator_))
      , buffer_(std::exchange(_other.buffer_, (VkBuffer)VK_NULL_HANDLE))
      , memory_(std::exchange(_other.memory_, (VkDeviceMemory)VK_NULL_HANDLE))
      , permanent_map_(std::exchange(_other.permanent_map_, nullptr)) {}
  buffer_page_data &operator=(buffer_page_data &&_other) noexcept {
    if (&_other != this) {
      parent_        = std::exchange(_other.parent_, nullptr);
      suballocator_  = std::move(_other.suballocator_);
      buffer_        = std::exchange(_other.buffer_, (VkBuffer)VK_NULL_HANDLE);
      memory_        = std::exchange(_other.memory_, (VkDeviceMemory)VK_NULL_HANDLE);
      permanent_map_ = std::exchange(_other.permanent_map_, nullptr);
    }
    return *this;
  }

  buffer_page_data(buffer &_parent, uint _size);

  void                             release_impl(buffer_suballoc<u8> *_suballoc);
  [[nodiscard]] buffer_updator<u8> update_raw_impl(uint _offset_bytes, uint _size_bytes);
  void                             finalize_impl(buffer_updator<u8> *_updator);
  void                             zero_raw(uint _offset_bytes, uint _size_bytes) const;

  template<typename TContained>
  void release(buffer_suballoc<TContained> *_suballoc) {
    release_impl(_suballoc->template reinterpret_as<u8>());
  }

  template<typename TContained>
  void finalize(buffer_updator<TContained> *_updator) {
    finalize_impl(_updator->template reinterpret_as<u8>());
  }

  template<typename TContained>
  [[nodiscard]] buffer_updator<TContained> update_raw(uint _offset_bytes, uint _size_bytes) {
    return std::move(*update_raw_impl(_offset_bytes, _size_bytes).template reinterpret_as<TContained>());
  }

  [[nodiscard]] uint size() const { return suballocator_.capacity(); }
};
}  // namespace details

struct buffer_params {
  bool                  permanent_map_     = false;
  uint                  initial_byte_size_ = 32 * 1024 * 1024;
  VkMemoryPropertyFlags type_              = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  VkBufferUsageFlagBits usage_             = VkBufferUsageFlagBits(
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                  | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
};

class buffer {
  friend struct details::buffer_page_data;

 public:
  buffer(display &_display, const buffer_params &_params = {});

  [[nodiscard]] buffer_suballoc<u8> allocate_raw(uint _size_bytes, uint _align = 4);

  template<typename TSubType>
  [[nodiscard]] shared_buffer_suballoc<TSubType> allocate(uint _count, uint _align = 4) {
    auto raw_alloc = allocate_raw(_count * sizeof(TSubType), _align);
    return std::make_shared<buffer_suballoc<TSubType>>(std::move(*raw_alloc.template reinterpret_as<TSubType>()));
  }

#ifdef HUT_DEBUG_STAGING
  void debug();
#endif

 protected:
  display      &display_;
  buffer_params params_;

  std::list<details::buffer_page_data> pages_;

  details::buffer_page_data &grow(uint _new_size) {
    return pages_.emplace_back(*this, _new_size);
  }
};

}  // namespace hut
