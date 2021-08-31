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

#include <bit>
#include <optional>

#include "hut/utils/math.hpp"

namespace hut::binpack {

template<typename T>
class linear1d {
 public:
  struct block {
    bool used_;
    T    offset_;
    T    size_;
  };

 private:
  using base_type = T;
  using type      = T;

  size_t             last_found_fit_ = 0;
  T                  pool_size_, allocated_size_ = 0;
  std::vector<block> blocks_;

  void emplace_at(size_t _index, bool _status, T _offset, T _size) {
    assert(_index <= blocks_.size());
    blocks_.emplace(blocks_.begin() + _index, block{_status, _offset, _size});
  }

  void erase_at(size_t _index) {
    assert(_index < blocks_.size());

    blocks_.erase(blocks_.begin() + _index);
  }

  void erase_range(size_t _ibegin, size_t _iend) {
    assert(_ibegin < blocks_.size());
    assert(_iend <= blocks_.size());

    blocks_.erase(blocks_.begin() + _ibegin, blocks_.begin() + _iend);
  }

  struct fit {
    size_t index_;
    T      offset_;
    T      aligned_offset_;
    T      aligned_size_;
  };

  [[nodiscard]] std::optional<fit> find_first_fit(T _size, T _align) const {
    const auto blocks_count = blocks_.size();
    fit        result;
    for (size_t i = 0; i < blocks_count; i++) {
      result.index_ = (last_found_fit_ + i) % blocks_count;  // Start from last found fit
      auto block    = blocks_[result.index_];

      if (block.used_) continue;  // In use
      const auto size = block.size_;
      if (size < _size)
        continue;  // Too small even without considering alignment

      result.offset_         = block.offset_;
      result.aligned_offset_ = align<uint>(result.offset_, _align);
      const auto align_bytes = result.aligned_offset_ - result.offset_;
      result.aligned_size_   = align<uint>(_size + align_bytes, _align);
      if (size >= result.aligned_size_) {
        return result;
      }
    }
    return {};
  }

  [[nodiscard]] std::optional<size_t> find_offset(T _offset) {
    const auto blocks_count = blocks_.size();
    for (size_t i = 0; i < blocks_count; i++) {
      if (blocks_[i].offset_ == _offset)
        return i;
    }
    return {};
  }

  T split(const fit &_fit) {
    // Compute size/offset for new block after split
    const auto split_block      = blocks_[_fit.index_];
    const auto new_block_size   = split_block.size_ - _fit.aligned_size_;
    const auto new_block_offset = split_block.offset_ + _fit.aligned_size_;

    const auto align_bytes = _fit.aligned_offset_ - _fit.offset_;
    if (align_bytes) {
      // Increase size of previous block, and offset of current, so that it's aligned
      blocks_[_fit.index_ - 1].size_ += align_bytes;
      blocks_[_fit.index_].offset_ = _fit.aligned_offset_;
    }
    blocks_[_fit.index_].used_ = true;
    blocks_[_fit.index_].size_ = _fit.aligned_size_;
    assert(_fit.aligned_size_ > 0);

    if (new_block_size > 0)
      emplace_at(_fit.index_ + 1, false, new_block_offset, new_block_size);

    return _fit.aligned_offset_;
  }

  void merge(size_t _index) {
    assert(blocks_[_index].used_ == true);
    blocks_[_index].used_       = false;
    const auto blocks_count     = blocks_.size();
    T          accumulated_size = blocks_[_index].size_;
    size_t     ibegin           = _index;
    size_t     iend             = _index;
    for (size_t i = _index + 1; i < blocks_count; i++) {
      if (blocks_[i].used_)
        break;
      accumulated_size += blocks_[i].size_;
      iend = i;
    }
    for (ssize_t i = ssize_t(_index) - 1; i >= 0; i--) {
      if (blocks_[i].used_)
        break;
      accumulated_size += blocks_[i].size_;
      ibegin = i;
    }
    if (ibegin == iend)
      return;

    blocks_[ibegin].size_ = accumulated_size;
    erase_range(ibegin + 1, iend + 1);
  }

 public:
  explicit linear1d(T _pool_size)
      : pool_size_(_pool_size) { reset(); }

  std::optional<T> pack(T _size, T _align = 0) {
    assert(_size > 0);
    auto first_fit = find_first_fit(_size, _align);
    if (!first_fit)
      return {};
    last_found_fit_ = first_fit->index_;
    allocated_size_ += _size;
    return split(*first_fit);
  }

  [[nodiscard]] bool try_fit(T _size, T _align = 0) const {
    return find_first_fit(_size, _align).has_value();
  }

  void offer(T _offset) {
    auto found = find_offset(_offset);
    assert(found);
    allocated_size_ -= blocks_[*found].size_;
    merge(*found);
  }

  void reset() {
    blocks_.clear();
    emplace_at(0, false, 0, pool_size_);
    last_found_fit_ = 0;
  }

  [[nodiscard]] T capacity() const { return pool_size_; }
  [[nodiscard]] T allocated() const { return allocated_size_; }
  [[nodiscard]] T free() const { return pool_size_ - allocated_size_; }

  [[nodiscard]] bool empty() const {
    assert(!blocks_.empty());
    return blocks_.size() == 1 && !blocks_[0].used_;
  }

  [[nodiscard]] T upper_bound() const {
    for (auto it = blocks_.crbegin(); it != blocks_.crend(); ++it) {
      if (it->used_)
        return it->offset_ + it->size_;
    }
    return 0;
  }

  template<typename TVisitor>
  void visit_blocks(TVisitor _visitor) const {
    for (const auto &block : blocks_)
      if (!_visitor(block))
        break;
  }
};

template<typename T, typename TUnderlying>
struct adaptor1d_dummy2d {
  // NOTE JBL: Dummy adapter to display a 1d as 2d

  TUnderlying underlying_;

  using base_type = T;
  using type      = vec<2, T>;

  explicit adaptor1d_dummy2d(const type &_size)
      : underlying_(_size.x) {}

  std::optional<type> pack(const type &_size) {
    auto result = underlying_.pack(_size.x);
    if (!result)
      return {};
    return type{*result, 0};
  }
  bool try_fit(const type &_size) { return underlying_.try_fit(_size.x); }
  void offer(const type &_free) { underlying_.offer(_free.x); }
  void reset() { underlying_.reset(); }
  bool empty() { return underlying_.empty(); }
};

template<typename T, T TAlignment>
struct shelve_separator_align {
  u16 select_shelve(u16 _size) {
    return align(_size, TAlignment);
  }
};

template<typename T, T TMin>
struct shelve_separator_pow {
  u16 select_shelve(u16 _size) {
    return std::max(std::bit_ceil(_size), u16(16));
  }
};

template<typename T, typename TShelveSelector>
struct shelve {
  // Uses a linear1d to maintain a shelves list, and another linear1d in each shelve to arrange elements horizontally
  using base_type = T;
  using type      = vec<2, T>;

  struct row {
    T           y_;
    linear1d<T> suballocator_;

    row(T _y, T _sizex)
        : y_(_y)
        , suballocator_(_sizex) {}
  };

  type                            size_;
  linear1d<T>                     shelves_allocator_;
  std::unordered_multimap<T, row> rows_;

  explicit shelve(const type &_size)
      : size_(_size)
      , shelves_allocator_(_size.y) { reset(); }

  std::optional<type> pack(const type &_size) {
    auto pow   = TShelveSelector{}.select_shelve(_size.y);
    auto range = rows_.equal_range(pow);
    for (auto it = range.first; it != range.second; it++) {
      auto fit = it->second.suballocator_.pack(_size.x);
      if (fit)
        return type{*fit, it->second.y_};
    }

    auto new_row_y = shelves_allocator_.pack(pow);
    if (!new_row_y)
      return {};

    auto new_row = rows_.emplace(pow, row{*new_row_y, size_.x});
    auto fit     = new_row->second.suballocator_.pack(_size.x);
    assert(fit);
    return type{*fit, new_row->second.y_};
  }

  void offer(const type &_free) {
    for (auto it = rows_.begin(); it != rows_.end(); it++) {
      if (it->second.y_ == _free.y) {
        auto &allocator = it->second.suballocator_;
        allocator.offer(_free.x);
        if (allocator.empty()) {
          shelves_allocator_.offer(_free.y);
          rows_.erase(it);
        }
        return;
      }
    }
  }

  void reset() {
    rows_.clear();
    shelves_allocator_.reset();
  }

  bool empty() {
    return shelves_allocator_.empty();
  }
};

}  // namespace hut::binpack
