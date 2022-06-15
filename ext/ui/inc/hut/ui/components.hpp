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

#include <entt/entt.hpp>

#include "hut/render2d/renderer.hpp"
#include "hut/text/renderer.hpp"

namespace hut::ui {

using bbox_px  = u16bbox_px;
using point_px = u16vec2_px;
using size_px  = u16vec2_px;

struct neighbors {
  entt::entity parent_;
  entt::entity next_;
  entt::entity prev_;
};

struct container {
  entt::entity first_;
  entt::entity last_;
};

struct invalid {};
struct dirty {};

struct boxes {
  render2d::boxes_holder holder_;
};

struct words {
  text::words_holder holder_;
};

struct bbox {
  bbox_px bbox_;
};

struct constraints {
  size_px min_;
  size_px max_;
};

struct preferred_size {
  size_px preferred_;
};

struct margin {
  u8vec4_px margin_size_;
};

struct padding {
  u8vec4_px margin_size_;
};

struct rectcut_p {};

struct rectcut_c {
  enum { TOP, LEFT, BOTTOM, RIGHT } side_;

  u16_px length_;
};

}  // namespace hut::ui
