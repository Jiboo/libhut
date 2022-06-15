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

#include "hut/ui/components.hpp"

namespace hut::ui {

struct root_params {
  shared_ubo ubo_;

  shared_atlas   words_atlas_;
  image_params   words_atlas_dparams_{.size_ = u16vec2_px{u16_px{(1 << 15) - 1}}, .format_ = VK_FORMAT_R8_UNORM};
  shared_sampler words_sampler_;
  sampler_params words_sampler_dparams_;

  text::renderer_params words_params_;

  shared_atlas   boxes_atlas_;
  image_params   boxes_atlas_dparams_{.size_ = u16vec2_px{u16_px{(1 << 16) - 1}}, .format_ = VK_FORMAT_R8G8B8A8_UNORM};
  shared_sampler boxes_sampler_;
  sampler_params boxes_sampler_dparams_;

  render2d::renderer_params boxes_params_;
};

class root {
 public:
  root(display &_dsp, window &_parent, shared_buffer _buf, text::shared_font _font, const root_params &_params = {});

 private:
  bool mouse(u8 _button, mouse_event_type _type, vec2 _coords);
  bool resize(const u16vec2_px &_size, u32 _scale);
  bool frame(display::duration _dur);
  bool draw(VkCommandBuffer _buff);

  display &display_;
  window  &parent_;

  shared_buffer buffer_;
  shared_ubo    ubo_;

  shared_sampler     boxes_sampler_;
  shared_atlas       boxes_atlas_;
  render2d::renderer boxes_renderer_;

  shared_sampler    words_sampler_;
  shared_atlas      words_atlas_;
  text::shared_font font_;
  text::renderer    words_renderer_;

  entt::registry registry_;
};

}  // namespace hut::ui
