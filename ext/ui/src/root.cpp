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

#include "hut/ui/root.hpp"

namespace hut::ui {

static shared_buffer default_buffer(display &_dsp, const buffer_params &_params) {
  return std::make_shared<buffer>(_dsp, _params);
}

static shared_atlas default_atlas(display &_dsp, const shared_buffer &_buf, const image_params &_params) {
  return std::make_shared<atlas>(_dsp, _buf, _params);
}

static shared_ubo default_ubo(display &_dsp, const shared_buffer &_buffer) {
  return _buffer->allocate<common_ubo>(1, _dsp.ubo_align());
}

static shared_sampler default_sampler(display &_dsp, const sampler_params &_params) {
  return std::make_shared<sampler>(_dsp, _params);
}

root::root(display &_dsp, window &_parent, shared_buffer _buf, text::shared_font _font, const root_params &_params)
    : display_(_dsp)
    , parent_(_parent)
    , buffer_(std::move(_buf))
    , ubo_(_params.ubo_ ? _params.ubo_ : default_ubo(_dsp, buffer_))
    , boxes_sampler_(_params.boxes_sampler_ ? _params.boxes_sampler_ : default_sampler(_dsp, _params.boxes_sampler_dparams_))
    , boxes_atlas_(_params.boxes_atlas_ ? _params.boxes_atlas_ : default_atlas(_dsp, buffer_, _params.boxes_atlas_dparams_))
    , boxes_renderer_(_parent, buffer_, ubo_, boxes_atlas_, boxes_sampler_, _params.boxes_params_)
    , words_sampler_(_params.words_sampler_ ? _params.words_sampler_ : default_sampler(_dsp, _params.words_sampler_dparams_))
    , words_atlas_(_params.words_atlas_ ? _params.words_atlas_ : default_atlas(_dsp, buffer_, _params.words_atlas_dparams_))
    , font_(std::move(_font))
    , words_renderer_(_parent, buffer_, font_, ubo_, words_atlas_, words_sampler_, _params.words_params_) {
  _parent.on_draw_.connect(std::bind_front(&ui::root::draw, this));
  _parent.on_frame_.connect(std::bind_front(&ui::root::frame, this));
  _parent.on_mouse_.connect(std::bind_front(&ui::root::mouse, this));
  _parent.on_resize_.connect(std::bind_front(&ui::root::resize, this));
}

bool root::frame(display::duration _dur) {
  return false;
}

bool root::draw(VkCommandBuffer _buff) {
  boxes_renderer_.draw(_buff);
  words_renderer_.draw(_buff);
  return false;
}

bool root::mouse(u8 _button, mouse_event_type _type, vec2 _coords) {
  constexpr float RESIZE_BORDER_THRESHOLD = 20;

  edge coords_edge;
  if (_coords.x < RESIZE_BORDER_THRESHOLD)
    coords_edge |= LEFT;
  if (_coords.y < RESIZE_BORDER_THRESHOLD)
    coords_edge |= TOP;
  if (float(parent_.size().x) - _coords.x < RESIZE_BORDER_THRESHOLD)
    coords_edge |= RIGHT;
  if (float(parent_.size().y) - _coords.y < RESIZE_BORDER_THRESHOLD)
    coords_edge |= BOTTOM;
  parent_.cursor(edge_cursor(coords_edge));

  static bool s_clicked = false;
  if (_type == MDOWN && _button == 1)
    s_clicked = true;
  else if (_type == MUP && _button == 1)
    s_clicked = false;
  else if (_type == MMOVE && s_clicked) {
    if (!coords_edge)
      parent_.interactive_move();
    else
      parent_.interactive_resize(coords_edge);
    return true;
  }
  return false;
}

bool root::resize(const u16vec2_px &_size, u32 _scale) {
  mat4 proj = ortho<float>(0.f, float(_size.x * _scale), 0.f, float(_size.y * _scale));
  ubo_->set_subone(0, offsetof(common_ubo, proj_), sizeof(mat4), &proj);

  float dpi_scale{static_cast<float>(_scale)};
  ubo_->set_subone(0, offsetof(common_ubo, dpi_scale_), sizeof(dpi_scale), &dpi_scale);
  return false;
}

}  //namespace hut::ui
