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

class buffer;

class buffer {
  friend class colored_triangle_list;

  display &display_;
  uint32_t size_;
  VkBuffer buffer_;
  VkDeviceMemory memory_;

  struct range_t {
    uint32_t offset_, size_;
    bool allocated_;

    bool operator==(const range_t &_other) const {
      return offset_ == _other.offset_ && size_ == _other.size_ &&
             allocated_ == _other.allocated_;
    }

    bool operator<(const range_t &_other) const {
      return offset_ < _other.offset_;
    }
  };
  std::set<range_t> ranges_;
  range_t do_alloc(uint32_t _size);
  void do_free(uint32_t _offset, uint32_t _size);
  void debug_ranges();

 public:
  /** Holds a reference to a zone in a buffer. */
  template <typename T, uint32_t TCount = 1>
  class ref {
    friend class buffer;
    friend class colored_triangle_list;

   public:
    buffer &buffer_;
    const uint32_t offset_, size_;

    ref(buffer &_buffer, uint32_t _offset, uint32_t _size)
        : buffer_(_buffer), offset_(_offset), size_(_size) {}

    ~ref() { buffer_.do_free(offset_, size_); }

    template <class TContainer>
    void set(const TContainer &_data) {
      assert(_data.size() == TCount);
      buffer_.update(offset_, size_, (void *)_data.begin().base());
    }
  };

  buffer(display &_display, uint32_t _size, VkBufferUsageFlagBits _usage);
  ~buffer();

  void update(uint32_t _offset, uint32_t _size, const void *_data);
  void resize(uint32_t _size);

  template <typename T, uint32_t TCount>
  std::shared_ptr<ref<T, TCount>> allocate() {
    range_t result = do_alloc(sizeof(T) * TCount);
    return std::make_shared<ref<T, TCount>>(*this, result.offset_,
                                            result.size_);
  }

  template <typename T, size_t TCount>
  void free(const ref<T, TCount> &_ref) {
    do_free(_ref.offset_, _ref.size_);
  };
};

template <typename T, uint32_t TCount>
using shared_ref = std::shared_ptr<buffer::ref<T, TCount>>;

}  // namespace hut
