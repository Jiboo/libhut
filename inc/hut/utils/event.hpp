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

#include <functional>
#include <optional>
#include <tuple>
#include <vector>

namespace hut {

template<typename... TArgTypes>
class event {
 public:
  using callback = std::function<bool(TArgTypes...)>;
  std::vector<callback> cbs_;

  void connect(const callback &_callback) { cbs_.emplace_back(_callback); }

  bool fire(const TArgTypes &..._args) {
    bool handled = false;

    for (auto &cb : cbs_) {
      handled |= cb(_args...);
      if (handled)
        break;
    }

    return handled;
  }

  void clear() { cbs_.clear(); }
};

template<typename... TArgTypes>
class buffered_event : public event<TArgTypes...> {
 public:
  std::optional<std::tuple<TArgTypes...>> args_;

  template<typename... TBufferArgTypes>
  void buffer(TBufferArgTypes &&..._args) {
    args_.emplace(std::forward<TBufferArgTypes>(_args)...);
  }

  bool pending() { return args_.has_value(); }

  bool flush() {
    assert(pending());
    bool result = std::apply(std::bind_front(&buffered_event::fire, this), *args_);
    args_.reset();
    return result;
  }
};

}  // namespace hut
