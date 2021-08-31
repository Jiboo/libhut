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

class suballoc_raw : public std::enable_shared_from_this<suballoc_raw> {
 public:
  class updator {
    friend class buffer;

    shared_suballoc_raw parent_alloc_;
    shared_suballoc_raw staging_alloc_;
    std::span<u8>       staging_;
    uint                offset_bytes_ = 0;

   public:
    updator()                = delete;
    updator(const updator &) = delete;

    updator(shared_suballoc_raw _parent, shared_suballoc_raw &&_staging_alloc, std::span<u8> _staging, uint _offset_bytes)
        : parent_alloc_(std::move(_parent))
        , staging_alloc_(_staging_alloc)
        , staging_(_staging)
        , offset_bytes_(_offset_bytes) {}
    updator(updator &&_other) = default;
    ~updator() {
      if (parent_alloc_) parent_alloc_->finalize(*this);
    }

    updator &operator=(const updator &) = delete;
    updator &operator=(updator &&) noexcept = default;

    [[nodiscard]] std::span<u8>       staging() { return staging_; };
    [[nodiscard]] std::span<const u8> staging() const { return staging_; };

    [[nodiscard]] const shared_suballoc_raw &parent_alloc() const { return parent_alloc_; };
    [[nodiscard]] const shared_suballoc_raw &staging_alloc() const { return staging_alloc_; };

    [[nodiscard]] uint total_offset_bytes() const { return parent_alloc_->offset_bytes() + offset_bytes_; }
    [[nodiscard]] uint offset_bytes() const { return offset_bytes_; }
    [[nodiscard]] uint size_bytes() const { return staging_.size_bytes(); }
  };

 public:
  suballoc_raw()                     = delete;
  suballoc_raw(const suballoc_raw &) = delete;
  suballoc_raw &operator=(const suballoc_raw &) = delete;

  suballoc_raw(uint _offset_bytes, uint _size_bytes)
      : offset_bytes_(_offset_bytes)
      , size_bytes_(_size_bytes) {}
  suballoc_raw(suballoc_raw &&_other) noexcept = default;
  suballoc_raw &operator=(suballoc_raw &&_other) noexcept = default;

  virtual ~suballoc_raw() = default;

  explicit operator bool() const { return valid(); }

  [[nodiscard]] uint offset_bytes() const { return offset_bytes_; }
  [[nodiscard]] uint size_bytes() const { return size_bytes_; }

  [[nodiscard]] virtual updator  update_raw(uint _offset_bytes, uint _size_bytes) = 0;
  virtual void                   finalize(const updator &_updator)                = 0;
  virtual void                   zero_raw(uint _offset_bytes, uint _size_bytes)   = 0;
  [[nodiscard]] virtual VkBuffer underlying_buffer() const                        = 0;
  [[nodiscard]] virtual u8 *     existing_mapping()                               = 0;
  [[nodiscard]] virtual bool     valid() const                                    = 0;
  virtual void                   release()                                        = 0;

 private:
  uint offset_bytes_ = 0, size_bytes_ = 0;
};

template<typename TContained, typename TSpecialisation>
class suballoc : public TSpecialisation {
 public:
  class updator : public TSpecialisation::updator {
   public:
    using parent_t = typename TSpecialisation::updator;
    using TSpecialisation::updator::updator;
    using TSpecialisation::updator::operator=;

    updator(parent_t &&_parent)
        : parent_t(std::move(_parent)) {}

    TContained &operator[](uint _index) { return *(reinterpret_cast<TContained *>(parent_t::staging().data()) + _index); }
    TContained *begin() { return reinterpret_cast<TContained *>(parent_t::staging().data()); }
    TContained *end() { return reinterpret_cast<TContained *>(parent_t::staging().data() + parent_t::staging().size()); }

    void set(std::span<const TContained> _input) {
      assert(_input.size_bytes() == parent_t::size_bytes());
      memcpy(parent_t::staging().data(), _input.data(), _input.size_bytes());
    }

    void set(std::initializer_list<TContained> _input) {
      set(std::span<const TContained>(std::data(_input), _input.size()));
    }
  };

 public:
  using parent_t = TSpecialisation;
  using TSpecialisation::TSpecialisation;
  using TSpecialisation::operator=;

  [[nodiscard]] uint offset() const { return parent_t::offset_bytes() / sizeof(TContained); }
  [[nodiscard]] uint size() const { return parent_t::size_bytes() / sizeof(TContained); }

  updator update(uint _offset, uint _size = 1) {
    return parent_t::update_raw(_offset * sizeof(TContained), _size * sizeof(TContained));
  }
  updator update() { return update(0, size()); }

  void zero(uint _offset, uint _size = 1) {
    parent_t::zero_raw(_offset * sizeof(TContained), _size * sizeof(TContained));
  }
  void zero() { zero(0, size()); }

  void set(std::span<const TContained> _data) { update().set(_data); }
  void set(std::initializer_list<TContained> _data) { update().set(_data); }
  void set(const TContained &_data) { set(std::span<const TContained>{&_data, 1}); }
  void set(TContained &&_data) { set(std::span<const TContained>{&_data, 1}); }

  void set_one(uint _offset, const TContained &_data) { update(_offset).set(std::span<const TContained>{&_data, 1}); }

  template<class TSubType>
  void set_subone(uint _offset, uint _byte_offset, uint _byte_size, const TSubType &_data) {
    auto    in_offset_bytes = _offset * sizeof(TContained) + _byte_offset;
    updator raw             = parent_t::update_raw(in_offset_bytes, _byte_size);
    memcpy(raw.staging().data(), _data, _byte_size);
  }
};

}  // namespace hut
