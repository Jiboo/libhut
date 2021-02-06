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

#include "hut/image.hpp"

namespace hut {

class atlas {
 public:
  atlas(display &_display, const image_params &_params);

  inline shared_image image() {
    return atlas_;
  }

  vec4 pack(const uvec2 &_bounds, uint8_t *_data, uint _src_row_pitch);

 private:
  // http://blackpawn.com/texts/lightmaps/default.html
  struct node {
    node *children_[2] {nullptr, nullptr};
    uvec4 coords_ {};

    ~node();

    inline bool leaf() { return children_[0] == nullptr && children_[1] == nullptr; }
  };

  shared_image atlas_;
  node root_;

  static node *binpack(node *_cur_node, const uvec2 &_bounds);
};

using shared_atlas = std::shared_ptr<atlas>;

}  // namespace hut
