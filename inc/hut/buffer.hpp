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

#include <memory>
#include <set>

#include <glm/glm.hpp>

#include "hut/display.hpp"

namespace hut {

class display;

class buffer {
  friend class display;
  friend class colored_triangle_list;
  friend class coltex_triangle_list;

 public:
  struct range_t {
    uint32_t offset_, size_;
    bool allocated_;

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
    const uint32_t offset_, size_;

    ref(buffer &_buffer, uint32_t _offset, uint32_t _size) : buffer_(_buffer), offset_(_offset), size_(_size) {
    }

    ~ref() {
      buffer_.do_free(offset_, size_);
    }

    void set(const std::initializer_list<T> &_data) {
      assert(_data.size() == size_ / sizeof(T));
      buffer_.update(offset_, size_, (void *)_data.begin());
    }

    template <class TContainer>
    void set(const TContainer &_data) {
      assert(_data.size() == size_ / sizeof(T));
      buffer_.update(offset_, size_, (void *)_data.begin().base());
    }

    uint32_t count() const {
      return size_ / sizeof(T);
    }
  };

  buffer(display &_display, uint32_t _size, VkMemoryPropertyFlags _type, VkBufferUsageFlagBits _usage);
  ~buffer();

  void update(uint32_t _offset, uint32_t _size, const void *_data);
  void copy_from(buffer &_other, uint32_t _other_offset, uint32_t _this_offset, uint32_t _size);

  template <typename T>
  std::shared_ptr<ref<T>> allocate(uint32_t _count = 1) {
    range_t result = do_alloc(sizeof(T) * _count);
    return std::make_shared<ref<T>>(*this, result.offset_, result.size_);
  }

  template <typename T>
  void free(const ref<T> &_ref) {
    do_free(_ref.offset_, _ref.size_);
  };

  bool operator==(const buffer &_other) const {
    return buffer_ == _other.buffer_;
  }

 private:
  display &display_;
  uint32_t size_;
  VkMemoryPropertyFlags type_;
  VkBufferUsageFlagBits usage_;
  VkBuffer buffer_;
  VkDeviceMemory memory_;

  std::set<range_t> ranges_;

  void grow(uint32_t new_size);
  range_t do_alloc(uint32_t _size);
  void do_free(uint32_t _offset, uint32_t _size);
  void merge();
  void debug_ranges();
  void clear_ranges();
};

template <typename T>
using shared_ref = std::shared_ptr<buffer::ref<T>>;

}  // namespace hut
