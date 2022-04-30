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

#include "hut/utils/math.hpp"
#include "hut/utils/profiling.hpp"

#include "hut/atlas.hpp"

namespace hut {

atlas::atlas(display &_display, const shared_buffer &_storage, const image_params &_params)
    : display_(_display)
    , storage_(_storage)
    , params_(_params) {
  HUT_PROFILE_FUN(PIMAGE)
  if (params_.size_ == u16vec2_px{0, 0}) {
    params_.size_ = {_display.limits().maxImageDimension2D, _display.limits().maxImageDimension2D};
  }
  add_page();
}

void atlas::add_page() {
  HUT_PROFILE_FUN(PIMAGE)
  pages_.emplace_back(std::make_shared<image>(display_, storage_, params_));
}

shared_subimage atlas::alloc(const u16vec2_px &_bounds) {
  HUT_PROFILE_FUN(PIMAGE, _bounds)
  assert(_bounds.x > 0_px);
  assert(_bounds.y > 0_px);
  auto padded = _bounds + PADDING;
  assert(padded.x < params_.size_.x);
  assert(padded.y < params_.size_.y);
  for (uint i = 0; i < pages_.size(); i++) {
    auto packed = pages_[i].packer_.pack(padded);
    if (packed)
      return std::make_shared<subimage>(this, i, make_bbox_with_origin_size(u16vec2_px{*packed}, _bounds));
  }
  add_page();
  auto packed = pages_.back().packer_.pack(padded);
  assert(packed.has_value());
  return std::make_shared<subimage>(this, pages_.size() - 1, make_bbox_with_origin_size(u16vec2_px{*packed}, _bounds));
}

shared_subimage atlas::pack(const u16vec2_px &_bounds, std::span<const u8> _data, uint _src_row_pitch) {
  HUT_PROFILE_FUN(PIMAGE, _bounds)
  shared_subimage result = alloc(_bounds);
  pages_[result->page()].image_->update({result->bounds()}, _data, _src_row_pitch);
  return result;
}

image::updator atlas::update(const subimage &_sub, const u16vec4_px &_bounds) {
  HUT_PROFILE_FUN(PIMAGE, _sub.page(), _sub.bounds(), _bounds)
  assert(_sub.from(this));
  return pages_[_sub.page()].image_->update({_bounds});
}

void atlas::free(subimage &&_sub) {
  HUT_PROFILE_FUN(PIMAGE, _sub.bounds())
  assert(_sub.from(this));
  auto &l = pages_[_sub.page()];
  l.packer_.offer(_sub.bounds());
}

}  // namespace hut
