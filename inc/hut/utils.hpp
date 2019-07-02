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

#include <codecvt>
#include <fstream>
#include <functional>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace hut {

inline std::string to_utf8(char32_t ch) {
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
  return convert.to_bytes(ch);
}

template <typename... TArgTypes>
class event {
 public:
  using callback = std::function<bool(TArgTypes...)>;

  void connect(const callback &_callback) {
    cbs_.emplace_back(_callback);
  }

  void once(const callback &_callback) {
    onces_.emplace_back(_callback);
  }

  bool fire(const TArgTypes &... _args) {
    bool handled = false;

    for (auto &cb : cbs_) {
      handled |= cb(_args...);
    }

    for (auto &cb : onces_) {
      handled |= cb(_args...);
    }
    onces_.clear();

    return handled;
  }

  void clear() {
    cbs_.clear();
  }

 protected:
  std::vector<callback> cbs_, onces_;
};

class sstream {
 private:
  std::ostringstream stream_;

 public:
  sstream(const char *_base) {
    stream_ << _base;
  }

  template <typename T>
  sstream &operator<<(const T &_rhs) {
    stream_ << _rhs;
    return *this;
  }

  std::string str() {
    return stream_.str();
  }

  const char* c_str() {
    return stream_.str().c_str();
  }

  operator std::string() {
    return str();
  }
};

/*static std::vector<char> read_file(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open())
    throw std::runtime_error(sstream("failed to open file: ") << filename);

  size_t size = (size_t)file.tellg();
  std::vector<char> buffer(size);
  file.seekg(0);
  file.read(buffer.data(), size);

  return buffer;
}*/

}  // namespace hut
