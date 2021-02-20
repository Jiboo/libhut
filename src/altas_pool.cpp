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

#include "hut/atlas_pool.hpp"

using namespace hut;

atlas_pool::subimage::~subimage() {
  if (valid())
    pool_->free(*this);
}

atlas_pool::atlas_pool(display &_display, const image_params &_params) : display_(_display), params_(_params) {
  HUT_PROFILE_SCOPE(PIMAGE, "atlas::atlas");
  if (params_.size_ == u16vec2{0, 0}) {
    params_.size_ = {_display.limits().maxImageDimension2D, _display.limits().maxImageDimension2D};
  }
  add_bin();
}

void atlas_pool::add_bin() {
  bins_.emplace_back(std::make_shared<hut::image>(display_, params_));
}

shared_subimage atlas_pool::alloc(const u16vec2 &_bounds) {
  assert(_bounds.x > 0);
  assert(_bounds.y > 0);
  auto padded = _bounds + padding;
  for (uint i = 0; i < bins_.size(); i++) {
    auto packed = bins_[i].packer_.pack(padded);
    if (packed)
      return std::make_shared<subimage>(this, i, make_bbox_with_origin_size(*packed, padded));
  }
  add_bin();
  auto packed = bins_.back().packer_.pack(padded);
  assert(packed.has_value());
  return std::make_shared<subimage>(this, bins_.size() - 1, make_bbox_with_origin_size(packed.value(), padded));
}

shared_subimage atlas_pool::pack(const u16vec2 &_bounds, uint8_t *_data, uint _src_row_pitch) {
  auto result = alloc(_bounds);
  auto &l = bins_[result->bin_index_];
  l.image_->update(result->bounds_ - u16vec4{0, 0, padding}, _data, _src_row_pitch);
  return result;
}

void atlas_pool::free(subimage &_subimage) {
  assert(_subimage.pool_ == this);
  auto &l = bins_[_subimage.bin_index_];
  l.packer_.offer(_subimage.bounds_);
}
