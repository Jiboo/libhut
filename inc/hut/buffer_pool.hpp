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

#include "hut/utils.hpp"
#include "hut/binpacks.hpp"

namespace hut {

class display;

class buffer_pool {
protected:
  friend class display;
  template<typename TIndexType, typename TVertexType, typename TUpdater> friend class shaper;
  friend class image;
  template<typename TUBO, typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments> friend class pipeline;

  struct buffer {
    buffer_pool &parent_;
    uint size_;
    binpack::linear1d<uint> suballocator_;
    VkBuffer buffer_;
    VkDeviceMemory memory_;
    uint8_t *permanent_map_;

    buffer(buffer_pool &_parent, uint _size) : parent_(_parent), size_(_size), suballocator_(_size) {}
  };

  struct alloc {
    buffer *buffer_;
    uint offset_, size_;

    void reset() {
      buffer_ = nullptr;
      size_ = 0;
    }
  };

 public:
  /** Advanced update usage, to write directly to staging memory. */
  class updator {
    friend buffer_pool;

    buffer &target_;
    alloc staging_;
    uint offset_;
    span<uint8_t> data_;

    updator(buffer &_target, alloc _staging, span<uint8_t> _data, uint _offset)
        : target_(_target), staging_(_staging), offset_(_offset), data_(_data) {
    }

  public:
    ~updator() { target_.parent_.finalize_update(*this); }

    uint8_t *data() { return data_.data(); }
    size_t size_bytes() { return data_.size_bytes(); }
  };

  /** Holds a reference to a zone in a buffer. */
  template <typename T>
  class ref {
    friend class image;
    friend class buffer_pool;
    template<typename TUBO, typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments> friend class pipeline;

    alloc alloc_;

    buffer &buff() { return *alloc_.buffer_; };
    buffer_pool &pool() { return alloc_.buffer_->parent_; };

  public:
    ref() = delete;
    ref(const ref&) = delete;
    explicit ref(alloc &_alloc) : alloc_(_alloc) {}
    ref(ref &&_other) noexcept {
      alloc_ = _other.alloc_;
      _other.alloc_.reset();
    }
    ~ref() {
      pool().do_free(alloc_);
    }

    ref &operator=(const ref&) = delete;
    ref &operator=(ref &&_other) noexcept {
      alloc_ = _other.alloc_;
      _other.alloc_.reset();
      return *this;
    }

    void update_raw(uint _byte_offset, uint _byte_size, void *_data) {
      assert(_byte_offset + _byte_size <= size_bytes());
      pool().do_update(buff(), alloc_.offset_ + _byte_offset, _byte_size, _data);
    }

    updator update_raw_indirect(uint _byte_offset, uint _byte_size) {
      assert(_byte_offset + _byte_size <= size_bytes());
      return pool().prepare_update(buff(), alloc_.offset_ + _byte_offset, _byte_size);
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
      update_raw(byte_offset, _byte_size, _data);
    }

    void set(const T &_data) {
      assert(size_bytes() == sizeof(T));
      update_one(0, _data);
    }

    void set(const std::initializer_list<T> &_data) {
      assert(_data.size() * sizeof(T) == size_bytes());
      update_some(0, _data.size(), _data.begin());
    }

    void zero(uint _indice, uint _count) {
      size_t byte_offset = _indice * sizeof(T);
      size_t byte_size = _count * sizeof(T);
      assert(byte_offset % 4U == 0);
      assert(byte_size % 4U == 0);
      assert(byte_offset + byte_size <= size_bytes());
      pool().do_zero(buff(), alloc_.offset_ + byte_offset, byte_size);
    }

    [[nodiscard]] uint offset_bytes() const { return alloc_.offset_; }
    [[nodiscard]] uint size_bytes() const { return alloc_.size_; }
    [[nodiscard]] uint size() const { return size_bytes() / sizeof(T); }
    [[nodiscard]] VkBuffer vkBuffer() { return alloc_.buffer_->buffer_; }
  };

  buffer_pool(display &_display, uint _size, VkMemoryPropertyFlags _type, uint _usage);
  ~buffer_pool();

  template <typename T>
  std::shared_ptr<ref<T>> allocate(uint _count, uint _align = 4) {
    uint size = sizeof(T) * _count;
    auto alloc = do_alloc(size, _align);
    return std::make_shared<ref<T>>(alloc);
  }

  void debug();

 protected:
  static constexpr VkMemoryPropertyFlags staging_type = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  static constexpr uint staging_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  display &display_;
  uint total_size_ = 0;
  VkMemoryPropertyFlags type_;
  uint usage_; // see VkBufferUsageFlagBits

  std::vector<buffer> buffers_;

  buffer *grow(uint new_size);
  void free_buffer(buffer &_buff);
  alloc do_alloc(uint _size, uint _align = 4);
  void do_free(const alloc &_alloc);
  void do_update(buffer &_buf, uint _offset, uint _size, const void *_data);
  updator prepare_update(buffer &_buf, uint _offset, uint _size);
  void finalize_update(updator &_update);
  void do_zero(buffer &_buf, uint _offset, uint _size);
  void clear_ranges();
};

using shared_buffer = std::shared_ptr<buffer_pool>;

template <typename T>
using shared_ref = std::shared_ptr<buffer_pool::ref<T>>;

}  // namespace hut
