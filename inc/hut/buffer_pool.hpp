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

#include <list>
#include <memory>
#include <mutex>
#include <set>

#include <vulkan/vulkan.h>

#include "hut/utils.hpp"

namespace hut {

class display;

class buffer_pool {
  friend class display;
  friend class shaper;
  friend class image;
  template<typename TDetails, typename... TExtraBindings> friend class drawable;

  struct range;

  struct alloc {
    VkBuffer buffer_;
    VkDeviceMemory memory_;
    uint offset_;
    range *node_;
  };

 public:
  /** Advance update usage, to write directly to staging memory. */
  class updator {
    friend buffer_pool;

    buffer_pool &parent_;
    VkBuffer target_;
    buffer_pool::alloc staging_;
    uint offset_;
    span<uint8_t> data_;

    updator(buffer_pool &_parent, VkBuffer _target, buffer_pool::alloc _staging, span<uint8_t> _data, uint _offset)
      : parent_(_parent), target_(_target), staging_(_staging), offset_(_offset), data_(_data) {
    }

  public:
    ~updator() { parent_.finalize_update(*this); }

    uint8_t *data() { return data_.data(); }
    size_t size_bytes() { return data_.size_bytes(); }
  };

  /** Holds a reference to a zone in a buffer. */
  template <typename T>
  class ref {
    friend class buffer_pool;
    range *range_;

   public:
    buffer_pool &pool_;
    VkBuffer buffer_;
    const uint offset_, byte_size_;

    ref(range *_range, buffer_pool &_pool, VkBuffer _buffer, uint _offset, uint _size)
        : range_(_range), pool_(_pool), buffer_(_buffer), offset_(_offset), byte_size_(_size) {
    }

    ~ref() {
      pool_.do_free(range_);
    }

    void update_raw(uint _byte_offset, uint _byte_size, void *_data) {
      assert(_byte_offset + _byte_size <= byte_size_);
      pool_.do_update(buffer_, offset_ + _byte_offset, _byte_size, (void *)_data);
    }

    updator update_raw_indirect(uint _byte_offset, uint _byte_size) {
      assert(_byte_offset + _byte_size <= byte_size_);
      return pool_.prepare_update(buffer_, offset_ + _byte_offset, _byte_size);
    }

    void update_some(uint _index_offset, uint _count, const T *_src) {
      const uint byte_offset = _index_offset * sizeof(T);
      const uint byte_size = _count * sizeof(T);
      update_raw(byte_offset, byte_size, (void *)_src);
    }

    void update_one(uint _index, const T &_src) {
      update_some(_index, 1, &_src);
    }

    void update_subone(uint _index_offset, uint _byte_offset, uint _byte_size, void *_data) {
      const uint byte_offset = _index_offset * sizeof(T) + _byte_offset;
      assert(_byte_size <= sizeof(T));
      update_raw(byte_offset, _byte_size, (void *)_data);
    }

    void set(const T &_data) {
      assert(byte_size_ == sizeof(T));
      update_one(0, _data);
    }

    void set(const std::initializer_list<T> &_data) {
      assert(_data.size() * sizeof(T) == byte_size_);
      update_some(0, _data.size(), _data.begin());
    }

    void zero(uint _indice, uint _count) {
      size_t byte_offset = _indice * sizeof(T);
      size_t byte_size = _count * sizeof(T);
      assert(byte_offset % 4U == 0);
      assert(byte_size % 4U == 0);
      assert(byte_offset + byte_size <= byte_size_);
      pool_.do_zero(buffer_, offset_ + byte_offset, byte_size);
    }

    uint size() const {
      return byte_size_ / sizeof(T);
    }

    uint size_bytes() const {
      return byte_size_;
    }
  };

  buffer_pool(display &_display, uint _size, VkMemoryPropertyFlags _type, uint _usage);
  ~buffer_pool();

  template <typename T>
  std::shared_ptr<ref<T>> allocate(uint _count, uint _align = 4) {
    uint size = sizeof(T) * _count;
    auto alloc = do_alloc(size, _align);
    return std::make_shared<ref<T>>(alloc.node_, *this, alloc.buffer_, alloc.offset_, size);
  }

  void debug();

 private:
  display &display_;
  uint total_size_ = 0;
  VkMemoryPropertyFlags type_;
  uint usage_; // see VkBufferUsageFlagBits

  struct range {
    range *prev_, *next_;
    bool allocated_;
    uint offset_, size_;
  };

  struct buffer {
    uint size_;
    VkBuffer buffer_;
    VkDeviceMemory memory_;
    range *root_;
  };

  std::mutex mutex_;
  std::vector<buffer> buffers_;

  buffer *grow(uint new_size);
  void free_buffer(buffer &_buff);
  alloc do_alloc(uint _size, uint _align = 4);
  void do_free(range *_ref);
  void do_update(VkBuffer _buf, uint _offset, uint _size, const void *_data);
  updator prepare_update(VkBuffer _buf, uint _offset, uint _size);
  void finalize_update(updator &_update);
  void do_zero(VkBuffer _buf, uint _offset, uint _size);
  void clear_ranges();
};

using shared_buffer = std::shared_ptr<buffer_pool>;

template <typename T>
using shared_ref = std::shared_ptr<buffer_pool::ref<T>>;

}  // namespace hut
