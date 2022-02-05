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

#include <chrono>
#include <ostream>
#include <ratio>

namespace hut {

using namespace std::literals::chrono_literals;

template<typename TInternalClock, typename TRep>
class diff_clock_wrapper {
 public:
  using internal                  = TInternalClock;
  using rep                       = TRep;
  using period                    = std::micro;
  using duration                  = std::chrono::duration<rep, period>;
  using time_point                = std::chrono::time_point<diff_clock_wrapper, duration>;
  static constexpr bool is_steady = TInternalClock::is_steady;

  static time_point now() {
    static const typename TInternalClock::time_point epoch = TInternalClock::now();
    return time_point{std::chrono::duration_cast<duration>(TInternalClock::now() - epoch)};
  }
};

template<typename TRep, typename TPeriod>
inline std::ostream &operator<<(std::ostream &_os, const std::chrono::duration<TRep, TPeriod> &_dur) {
  auto nano = std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(_dur).count();
  if (nano < 1000)
    return _os << nano << "ns";
  else if (nano < 1'000'000)
    return _os << nano / 1000.0 << "µs";
  else if (nano < 1'000'000'000)
    return _os << nano / 1'000'000.0 << "ms";
  return _os << nano / 1'000'000'000.0 << "s";
}

template<typename TClock, typename TDur>
inline std::ostream &operator<<(std::ostream &_os, const std::chrono::time_point<TClock, TDur> &_tp) {
  auto elapsed = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(_tp.time_since_epoch());
  return _os << elapsed.count();
}

}  // namespace hut
