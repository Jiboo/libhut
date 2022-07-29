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

#include "hut/render2d/renderer.hpp"
#include "hut/text/renderer.hpp"
#include "hut/ui/common.hpp"

namespace hut::ui {

struct dirty {};

struct neighbors {
  entt::entity parent_ = entt::null;
  entt::entity next_   = entt::null;
  entt::entity prev_   = entt::null;
};

struct container {
  entt::entity first_ = entt::null;
  entt::entity last_  = entt::null;
};

struct bounds {
  bbox_px bbox_{0_px};
};

struct rectcut_length {
  side   side_;
  u16_px length_;
};

struct rectcut_ratio {
  side  side_;
  float ratio_;
};

struct updators {
  render2d::batch_updators boxes_updators_;
  text::batch_updators words_updators_;
};

using boxes = render2d::boxes_holder;
using words = text::words_holder;

using layout_handler = std::function<void(bbox_px, updators&)>;
using frame_handler  = std::function<bool(display::duration)>;
using mouse_handler  = std::function<bool(u8, mouse_event_type, vec2)>;

}  // namespace hut::ui
