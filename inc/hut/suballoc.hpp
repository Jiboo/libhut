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

#include <cstring>

#include <span>

namespace hut {

template<typename TContained, typename TParent>
class suballoc {
  friend struct details::buffer_page_data;

 public:
  using contained_t   = TContained;
  using parent_t      = TParent;
  using updator_t     = buffer_updator<TContained>;
  using raw_updator_t = updator<u8, TParent>;

  suballoc() = delete;
  ~suballoc() { release(); }

  suballoc(const suballoc &)            = delete;
  suballoc &operator=(const suballoc &) = delete;

  suballoc(suballoc &&_other) noexcept
      : parent_(std::exchange(_other.parent_, nullptr))
      , offset_bytes_(_other.offset_bytes_)
      , size_bytes_(_other.size_bytes_) {}
  suballoc &operator=(suballoc &&_other) noexcept {
    if (&_other != this) {
      if (parent_)
        parent_->release(this);
      parent_       = std::exchange(_other.parent_, nullptr);
      offset_bytes_ = _other.offset_bytes_;
      size_bytes_   = _other.size_bytes_;
    }
    return *this;
  }

  suballoc(TParent *_parent, uint _offset_bytes, uint _size_bytes)
      : parent_(_parent)
      , offset_bytes_(_offset_bytes)
      , size_bytes_(_size_bytes) {}

  template<typename TOtherContained>
  suballoc<TOtherContained, TParent> *reinterpret_as() {
    return reinterpret_cast<suballoc<TOtherContained, TParent> *>(this);
  }

  void release() {
    if (parent_)
      parent_->release(this);
    parent_ = nullptr;
  }

  [[nodiscard]] TParent *parent() const { return parent_; }

  [[nodiscard]] uint offset_bytes() const { return offset_bytes_; }
  [[nodiscard]] uint size_bytes() const { return size_bytes_; }

  [[nodiscard]] uint offset() const { return offset_bytes() / sizeof(TContained); }
  [[nodiscard]] uint size() const { return size_bytes() / sizeof(TContained); }

  updator_t update_raw(uint _offset_bytes, uint _size_bytes) {
    assert(_offset_bytes + _size_bytes <= size_bytes_);
    return parent_->template update_raw<TContained>(offset_bytes_ + _offset_bytes, _size_bytes);
  }
  updator_t update(uint _offset, uint _size = 1) {
    return update_raw(_offset * sizeof(TContained), _size * sizeof(TContained));
  }
  updator_t update() { return update(0, size()); }

  void zero_raw(uint _offset_bytes, uint _size_bytes) {
    assert(_offset_bytes + _size_bytes <= size_bytes_);
    return parent_->zero_raw(offset_bytes_ + _offset_bytes, _size_bytes);
  }
  void zero(uint _offset, uint _size = 1) { zero_raw(_offset * sizeof(TContained), _size * sizeof(TContained)); }
  void zero() { zero(0, size()); }

  void set(std::span<const TContained> _data) { update().set(_data); }
  void set(std::initializer_list<TContained> _data) { update().set(_data); }
  void set(const TContained &_data) { set(std::span<const TContained>{&_data, 1}); }
  void set(TContained &&_data) { set(std::span<const TContained>{&_data, 1}); }

  void set_one(uint _offset, const TContained &_data) { update(_offset).set(std::span<const TContained>{&_data, 1}); }

  template<class TSubType>
  void set_subone(uint _offset, uint _byte_offset, uint _byte_size, const TSubType &_data) {
    auto in_offset_bytes = _offset * sizeof(TContained) + _byte_offset;
    auto raw             = update_raw(in_offset_bytes, _byte_size);
    memcpy(raw.staging().data(), _data, _byte_size);
  }

 protected:
  TParent *parent_       = nullptr;
  u32      offset_bytes_ = 0;
  u32      size_bytes_   = 0;
};

template<typename TContained, typename TParent>
class updator {
 public:
  friend struct details::buffer_page_data;
  using staging_alloc_t = buffer_suballoc<TContained>;

 protected:
  TParent        *parent_ = nullptr;
  staging_alloc_t staging_alloc_;
  std::span<u8>   staging_;
  uint            offset_bytes_ = 0;

 public:
  updator() = delete;
  ~updator() {
    if (parent_)
      parent_->finalize(this);
  }

  updator(const updator &)            = delete;
  updator &operator=(const updator &) = delete;

  updator(updator &&_other) noexcept
      : parent_(std::exchange(_other.parent_, nullptr))
      , staging_alloc_(std::move(_other.staging_alloc_))
      , staging_(_other.staging_)
      , offset_bytes_(_other.offset_bytes_) {}
  updator &operator=(updator &&_other) noexcept {
    if (&_other != this) {
      parent_        = std::exchange(_other.parent_, nullptr);
      staging_alloc_ = std::move(_other.staging_alloc_);
      staging_       = _other.staging_;
      offset_bytes_  = _other.offset_bytes_;
    }
    return *this;
  }

  updator(TParent &_parent, staging_alloc_t &&_staging_alloc, std::span<u8> _staging, uint _offset_bytes)
      : parent_(&_parent)
      , staging_alloc_(std::move(_staging_alloc))
      , staging_(_staging)
      , offset_bytes_(_offset_bytes) {}

  template<typename TOtherContained>
  updator<TOtherContained, TParent> *reinterpret_as() {
    return reinterpret_cast<updator<TOtherContained, TParent> *>(this);
  }

  [[nodiscard]] TParent *parent() { return parent_; }
  [[nodiscard]] std::span<u8> staging() { return staging_; };
  [[nodiscard]] uint          total_offset_bytes() const { return parent_->offset_bytes() + offset_bytes_; }

  [[nodiscard]] uint offset_bytes() const { return offset_bytes_; }
  [[nodiscard]] uint size_bytes() const { return staging_.size_bytes(); }

  [[nodiscard]] uint offset() const { return offset_bytes() / sizeof(TContained); }
  [[nodiscard]] uint size() const { return size_bytes() / sizeof(TContained); }

  [[nodiscard]] TContained &operator[](uint _index) { return *(reinterpret_cast<TContained *>(staging_.data()) + _index); }
  [[nodiscard]] TContained *begin() { return reinterpret_cast<TContained *>(staging_.data()); }
  [[nodiscard]] TContained *end() { return reinterpret_cast<TContained *>(staging_.data() + staging_.size()); }

  void set(std::span<const TContained> _input) {
    assert(_input.size_bytes() == size_bytes());
    memcpy(staging_.data(), _input.data(), _input.size_bytes());
  }

  void set(std::initializer_list<TContained> _input) {
    set(std::span<const TContained>(std::data(_input), _input.size()));
  }
};

}  // namespace hut
