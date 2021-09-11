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

struct buffer_params {
  bool                  permanent_map_ = false;
  VkMemoryPropertyFlags type_          = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  VkBufferUsageFlagBits usage_         = VkBufferUsageFlagBits(
              VK_BUFFER_USAGE_TRANSFER_DST_BIT
              | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
              | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
              | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
              | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
              | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
};

class buffer {
 public:
  class buffer_suballoc : public suballoc_raw {
    buffer *parent_ = nullptr;
    uint    page_;

   public:
    buffer_suballoc() = delete;

    buffer_suballoc(const buffer_suballoc &) = delete;
    buffer_suballoc &operator=(const buffer_suballoc &) = delete;

    buffer_suballoc(buffer_suballoc &&_other) noexcept = default;
    buffer_suballoc &operator=(buffer_suballoc &&_other) noexcept = default;

    buffer_suballoc(buffer *_parent, uint _page, uint _offset_bytes, uint _size_bytes)
        : suballoc_raw(_offset_bytes, _size_bytes)
        , parent_(_parent)
        , page_(_page) {}
    ~buffer_suballoc() override {
      if (parent_) buffer_suballoc::release();
    }

    updator  update_raw(uint _offset_bytes, uint _size_bytes) final;
    void     finalize(const updator &_updator) final;
    void     zero_raw(uint _offset_bytes, uint _size_bytes) final;
    VkBuffer underlying_buffer() const final;
    u8      *existing_mapping() final;
    bool     valid() const final { return parent_ != nullptr; }
    void     release() final;
  };
  template<typename T> using suballoc        = suballoc<T, buffer_suballoc>;
  template<typename T> using shared_suballoc = std::shared_ptr<suballoc<T>>;

 public:
  buffer(display &_display, uint _size, const buffer_params &_params = {});

  shared_suballoc_raw                     allocate_raw(uint _size_bytes, uint _align = 4);
  template<typename T> shared_suballoc<T> allocate(uint _count, uint _align = 4) {
    return std::static_pointer_cast<suballoc<T>>(allocate_raw(_count * sizeof(T), _align));
  }

#ifdef HUT_DEBUG_STAGING
  void debug();
#endif

 protected:
  display      &display_;
  uint          size_ = 0;
  buffer_params params_;

  struct page_data {
    buffer                 *parent_ = nullptr;
    binpack::linear1d<uint> suballocator_;
    VkBuffer                buffer_        = VK_NULL_HANDLE;
    VkDeviceMemory          memory_        = VK_NULL_HANDLE;
    u8                     *permanent_map_ = nullptr;

    page_data() = delete;

    page_data(const page_data &) = delete;
    page_data &operator=(const page_data &) = delete;

    page_data(page_data &&_other) noexcept
        : parent_(std::exchange(_other.parent_, nullptr))
        , suballocator_(std::move(_other.suballocator_))
        , buffer_(std::exchange(_other.buffer_, (VkBuffer)VK_NULL_HANDLE))
        , memory_(std::exchange(_other.memory_, (VkDeviceMemory)VK_NULL_HANDLE))
        , permanent_map_(std::exchange(_other.permanent_map_, nullptr)) {}
    page_data &operator=(page_data &&_other) noexcept {
      if (&_other != this) {
        parent_        = std::exchange(_other.parent_, nullptr);
        suballocator_  = std::move(_other.suballocator_);
        buffer_        = std::exchange(_other.buffer_, (VkBuffer)VK_NULL_HANDLE);
        memory_        = std::exchange(_other.memory_, (VkDeviceMemory)VK_NULL_HANDLE);
        permanent_map_ = std::exchange(_other.permanent_map_, nullptr);
      }
      return *this;
    }

    page_data(buffer &_parent, uint _size);
    ~page_data();

    [[nodiscard]] uint size() const { return suballocator_.capacity(); }
  };
  std::vector<page_data> pages_;

  page_data &grow(uint new_size) { return pages_.emplace_back(*this, new_size); }
};

}  // namespace hut
