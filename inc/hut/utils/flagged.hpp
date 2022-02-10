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

#include <algorithm>
#include <bit>
#include <initializer_list>
#include <ostream>

#include "hut/utils/math.hpp"

namespace hut {

template<typename TEnum, TEnum TEnd, typename TUnderlying = u32>
class flagged {
 private:
  TUnderlying active_ = 0;

  constexpr explicit flagged(TUnderlying _init)
      : active_(_init) {}

 public:
  constexpr static size_t      UNDERLYING_BITS = sizeof(TUnderlying) * 8;
  constexpr static TUnderlying mask(TEnum _flag) { return 1U << _flag; }
  static_assert(std::bit_width(mask(TEnd)) <= UNDERLYING_BITS, "underlying type too small to hold values to end");

  using enum_type                       = TEnum;
  using underlying_type                 = TUnderlying;
  constexpr static TUnderlying ENUM_END = TEnd;

  constexpr flagged()                      = default;
  constexpr flagged(const flagged &_other) = default;
  constexpr explicit flagged(TEnum _init) { set(_init); }
  constexpr flagged(std::initializer_list<TEnum> _inits) {
    for (auto flag : _inits)
      set(flag);
  }

  constexpr flagged &operator=(const flagged &_other) {
    active_ = _other.active_;
    return *this;
  }
  constexpr flagged &operator=(TEnum _init) {
    set(_init);
    return *this;
  }

  [[nodiscard]] constexpr bool query(TEnum _flag) const { return active_ & mask(_flag); }
  [[nodiscard]] constexpr bool operator[](TEnum _flag) const { return query(_flag); }

  [[nodiscard]] constexpr size_t   count() const { return (size_t)std::popcount(active_); }
  [[nodiscard]] constexpr bool     empty() const { return active_ == 0; }
  [[nodiscard]] constexpr explicit operator bool() const { return active_ != 0; }

  constexpr void reset(TEnum _flag) { active_ &= ~mask(_flag); }
  constexpr void flip(TEnum _flag) { active_ ^= mask(_flag); }
  constexpr void set(TEnum _flag) { active_ |= mask(_flag); }

  constexpr void        clear() { active_ = 0; }
  constexpr TUnderlying raw() { return active_; }

  constexpr flagged  operator|(TEnum _flag) const { return flagged{active_ | mask(_flag)}; }
  constexpr flagged &operator|=(TEnum _flag) {
    active_ |= mask(_flag);
    return *this;
  }
  constexpr flagged  operator&(TEnum _flag) const { return flagged{active_ & mask(_flag)}; }
  constexpr flagged &operator&=(TEnum _flag) {
    active_ &= mask(_flag);
    return *this;
  }

  constexpr flagged  operator|(const flagged &_other) const { return flagged{active_ | _other.active_}; }
  constexpr flagged &operator|=(const flagged &_other) {
    active_ |= _other.active_;
    return *this;
  }
  constexpr flagged  operator&(const flagged &_other) const { return flagged{active_ & _other.active_}; }
  constexpr flagged &operator&=(const flagged &_other) {
    active_ &= _other.active_;
    return *this;
  }

  constexpr bool operator==(TEnum _flag) const { return active_ == mask(_flag); }
  constexpr bool operator==(const flagged &_other) const { return active_ == _other.active_; }

  struct const_iterator {
    TUnderlying shifted_;
    TUnderlying current_;

    constexpr const_iterator &operator++() {
      shifted_ = shifted_ >> 1U;
      current_++;
      auto to_skip = std::countr_zero(shifted_);
      shifted_     = to_skip == UNDERLYING_BITS ? 0 : shifted_ >> to_skip;
      current_     = std::min(TUnderlying(UNDERLYING_BITS), TUnderlying(current_ + to_skip));
      return *this;
    }
    constexpr bool  operator!=(const const_iterator &_other) const { return current_ != _other.current_; }
    constexpr TEnum operator*() const { return static_cast<TEnum>(current_); }
  };

  constexpr const_iterator cbegin() const {
    TUnderlying to_skip = std::countr_zero(active_);
    return const_iterator{active_ >> to_skip, to_skip};
  }
  constexpr const_iterator begin() const { return cbegin(); }
  constexpr const_iterator cend() const { return const_iterator{0, UNDERLYING_BITS}; }
  constexpr const_iterator end() const { return cend(); }
};

template<typename TEnum, TEnum TEnd, typename TUnderlying = u32>
inline std::ostream &operator<<(std::ostream &_os, flagged<TEnum, TEnd, TUnderlying> _flags) {
  _os << "(";
  for (auto it = _flags.cbegin(); it != _flags.end(); ++it) {
    _os << *it;
    if (auto next = it; ++next != _flags.end())
      _os << ",";
  }
  return _os << ")";
}

}  // namespace hut
