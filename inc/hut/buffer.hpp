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

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

namespace hut {

class display;

class buffer {
  friend class display;
  friend class shaper;
  friend class image;
  friend struct buffer_ubo;
  template<typename TDetails, typename... TExtraBindings> friend class drawable;

 public:
  struct range_t {
    uint offset_, size_;
    bool allocated_;

    range_t() {}
    range_t(uint _offset, uint _size, bool _allocated)
      : offset_(_offset), size_(_size), allocated_(_allocated) {}

    bool operator==(const range_t &_other) const {
      return offset_ == _other.offset_ && size_ == _other.size_ && allocated_ == _other.allocated_;
    }

    bool operator<(const range_t &_other) const {
      return offset_ < _other.offset_;
    }
  };

  /** Holds a reference to a zone in a buffer. */
  template <typename T>
  class ref {
    friend class buffer;

   public:
    buffer &buffer_;
    const uint offset_, size_;

    ref(buffer &_buffer, uint _offset, uint _size) : buffer_(_buffer), offset_(_offset), size_(_size) {
    }

    ~ref() {
      buffer_.do_free(offset_);
    }

    void set(const T *_data) {
      buffer_.update(offset_, size_, (void *)_data);
    }

    void set(const std::initializer_list<T> &_data) {
      const uint byte_size = _data.size() * sizeof(T);
      assert(byte_size == size_);
      set(_data.begin());
    }

    void update(uint _offset, const T &_src) {
      const uint byte_offset = _offset * sizeof(T);
      assert(byte_offset <= size_);
      buffer_.update(offset_ + byte_offset, sizeof(T), (void *)&_src);
    }

    void update(uint _offset, uint _byte_offset, uint _byte_size, void *_data) {
      const uint byte_offset = _offset * sizeof(T) + _byte_offset;
      assert(byte_offset + _byte_size <= size_);
      buffer_.update(offset_ + byte_offset, _byte_size, (void *)_data);
    }

    uint size() const {
      return size_ / sizeof(T);
    }
  };

  buffer(display &_display, uint _size, VkMemoryPropertyFlags _type, uint _usage);
  ~buffer();

  void update(uint _offset, uint _size, const void *_data);
  void debug_ranges();

  template <typename T>
  std::shared_ptr<ref<T>> allocate(uint _count, uint _align = 4) {
    uint size = sizeof(T) * _count;
    uint offset = do_alloc(size, _align);
    return std::make_shared<ref<T>>(*this, offset, size);
  }

  template <typename T>
  void free(const ref<T> &_ref) {
    do_free(_ref.offset_);
  };

  bool operator==(const buffer &_other) const {
    return buffer_ == _other.buffer_;
  }

 private:
  display &display_;
  uint size_;
  VkMemoryPropertyFlags type_;
  uint usage_; // see VkBufferUsageFlagBits
  VkBuffer buffer_;
  VkDeviceMemory memory_;

  std::list<range_t> ranges_;
  std::mutex ranges_mutex_;

  void init(uint _size, VkMemoryPropertyFlags _type, uint _usage);
  void grow(uint new_size);
  uint do_alloc(uint _size, uint8_t _align = 4);
  void do_free(uint _offset);
  void merge();
  void clear_ranges();
};

using shared_buffer = std::shared_ptr<buffer>;

template <typename T>
using shared_ref = std::shared_ptr<buffer::ref<T>>;

}  // namespace hut
