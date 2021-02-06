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

#include "hut/atlas.hpp"

using namespace hut;

atlas::atlas(display &_display, const image_params &_params) {
  HUT_PROFILE_SCOPE(PIMAGE, "atlas::atlas");
  auto params = _params;
  if (params.size_ == uvec2{0, 0}) {
    params.size_ = {_display.limits().maxImageDimension2D, _display.limits().maxImageDimension2D};
  }
  atlas_ = std::make_shared<hut::image>(_display, params);
  root_.coords_ = {0, 0, params.size_};
}

atlas::node::~node() {
  for (auto & i : children_) {
    delete i;
  }
}

vec4 atlas::pack(const uvec2 &_bounds, uint8_t *_data, uint _src_row_pitch) {
  const uvec2 padding = {2, 2};
  node *packed = binpack(&root_, _bounds + padding);
  if (packed == nullptr) {
    throw std::runtime_error("altas overflow");
  }

  atlas_->update(packed->coords_ - uvec4{0, 0, padding}, _data, _src_row_pitch);

  vec4 result;
  result[0] = float(packed->coords_[0]) / atlas_->size().x;
  result[1] = float(packed->coords_[1]) / atlas_->size().y;
  result[2] = float(packed->coords_[2] - padding.x) / atlas_->size().x;
  result[3] = float(packed->coords_[3] - padding.y) / atlas_->size().y;
  return result;
}

atlas::node *atlas::binpack(node *_cur_node, const uvec2 &_bounds) {
  if (!_cur_node->leaf()) {
    for (auto & i : _cur_node->children_) {
      node *new_node = binpack(i, _bounds);
      if (new_node)
        return new_node;
    }
    return nullptr;
  }

  uint node_w = _cur_node->coords_[2] - _cur_node->coords_[0];
  uint node_h = _cur_node->coords_[3] - _cur_node->coords_[1];
  if (_bounds.x > node_w || _bounds.y > node_h)
    return nullptr;

  uint new_node_w = node_w - _bounds.x;
  uint new_node_h = node_h - _bounds.y;
  node *left = _cur_node->children_[0] = new node;
  node *right = _cur_node->children_[1] = new node;
  if (new_node_w <= new_node_h) {
    left->coords_[0] = _cur_node->coords_[0] + _bounds.x;
    left->coords_[1] = _cur_node->coords_[1];
    left->coords_[2] = left->coords_[0] + new_node_w;
    left->coords_[3] = left->coords_[1] + _bounds.y;

    right->coords_[0] = _cur_node->coords_[0];
    right->coords_[1] = _cur_node->coords_[1] + _bounds.y;
    right->coords_[2] = right->coords_[0] + node_w;
    right->coords_[3] = right->coords_[1] + new_node_h;
  }
  else {
    left->coords_[0] = _cur_node->coords_[0];
    left->coords_[1] = _cur_node->coords_[1] + _bounds.y;
    left->coords_[2] = left->coords_[0] + _bounds.x;
    left->coords_[3] = left->coords_[1] + new_node_h;

    right->coords_[0] = _cur_node->coords_[0] + _bounds.x;
    right->coords_[1] = _cur_node->coords_[1];
    right->coords_[2] = right->coords_[0] + new_node_w;
    right->coords_[3] = right->coords_[1] + node_h;
  }

  _cur_node->coords_[2] = _cur_node->coords_[0] + _bounds.x;
  _cur_node->coords_[3] = _cur_node->coords_[1] + _bounds.y;
  return _cur_node;
}
