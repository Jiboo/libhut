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

#include "hut/subimage.hpp"

#include "hut/atlas.hpp"

namespace hut {

void subimage::release() {
  HUT_PROFILE_FUN(PIMAGE)
  if (atlas_ != nullptr)
    atlas_->free(std::move(*this));
  atlas_ = nullptr;
}

vec4 subimage::texcoords() const {
  vec4 result;
  vec2 atlas_size = atlas_->size();
  result[0]       = float(bounds_[0]) / atlas_size.x;
  result[1]       = float(bounds_[1]) / atlas_size.y;
  result[2]       = float(bounds_[2]) / atlas_size.x;
  result[3]       = float(bounds_[3]) / atlas_size.y;
  return result;
}

image::updator subimage::update(u16vec4_px _bounds) {
  HUT_PROFILE_FUN(PIMAGE)
  auto update_size     = bbox_size(_bounds);
  auto update_origin   = bbox_origin(_bounds);
  auto subimage_origin = bbox_origin(bounds_);
  auto image_bounds    = make_bbox_with_origin_size(subimage_origin + update_origin, update_size);
  return atlas_->update(*this, image_bounds);
}

}  // namespace hut
