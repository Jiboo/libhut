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

#include "hut/ui/components.hpp"

namespace hut::ui {

static shared_buffer default_buffer(display &_dsp, const buffer_params &_params) {
  return std::make_shared<buffer>(_dsp, _params);
}

static shared_atlas default_atlas(display &_dsp, const shared_buffer &_buf, const image_params &_params) {
  return std::make_shared<atlas>(_dsp, _buf, _params);
}

static shared_sampler default_sampler(display &_dsp, const sampler_params &_params) {
  return std::make_shared<sampler>(_dsp, _params);
}

root::root(display &_dsp, window &_parent, shared_buffer _buf, shared_ubo _ubo, text::shared_font _font, const root_params &_params)
    : display_(_dsp)
    , parent_(_parent)
    , buffer_(std::move(_buf))
    , ubo_(std::move(_ubo))
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
  entity_ = create_root();
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

  bbox_px bounds = {0, 0, _size};
  layout(entity_, bounds);
  return false;
}

entity root::create_root() {
  entt::entity result = registry_.create();
  registry_.emplace<neighbors>(result);
  registry_.emplace<container>(result);
  registry_.emplace<bounds>(result);
  registry_.emplace<rectcut_ratio>(result, LEFT, 1.f);
  return result;
}

void root::inject_back_child(entity _parent, entity _child) {
  assert(registry_.valid(_parent));
  auto &parent_container = assert_get<container>(_parent);
  auto &child_neighbors   = registry_.get_or_emplace<neighbors>(_child);
  child_neighbors.parent_ = _parent;

  if (parent_container.last_ == null) {
    assert(parent_container.first_ == null);
    parent_container.first_ = parent_container.last_ = _child;
  }
  else {
    assert(registry_.valid(parent_container.last_));
    child_neighbors.prev_ = parent_container.last_;
    auto &last_neighbors  = assert_get<neighbors>(parent_container.last_);
    assert(last_neighbors.next_ == null);
    last_neighbors.next_ = _child;
    parent_container.last_ = _child;
  }
}

void root::inject_front_child(entity _parent, entity _child) {
  assert(registry_.valid(_parent));
  auto &parent_container = assert_get<container>(_parent);
  auto &child_neighbors   = registry_.get_or_emplace<neighbors>(_child);
  child_neighbors.parent_ = _parent;

  if (parent_container.first_ == null) {
    assert(parent_container.last_ == null);
    parent_container.first_ = parent_container.last_ = _child;
  }
  else {
    assert(registry_.valid(parent_container.first_));
    child_neighbors.next_ = parent_container.first_;
    auto &first_neighbors = assert_get<neighbors>(parent_container.first_);
    assert(first_neighbors.prev_ == null);
    first_neighbors.prev_ = _child;
    parent_container.first_ = _child;
  }
}

void root::inject_after(entity _lsibling, entity _child) {
  assert(registry_.valid(_lsibling));
  auto &sibling_neighbors = assert_get<neighbors>(_lsibling);
  auto &parent_container  = assert_get<container>(sibling_neighbors.parent_);
  auto &child_neighbors   = registry_.get_or_emplace<neighbors>(_child);

  child_neighbors.parent_ = sibling_neighbors.parent_;
  child_neighbors.next_ = sibling_neighbors.next_;
  if (child_neighbors.next_ != null)
    assert_get<neighbors>(child_neighbors.next_).prev_ = _child;
  child_neighbors.prev_ = _lsibling;
  sibling_neighbors.next_ = _child;

  if (parent_container.last_ == _lsibling)
    parent_container.last_ = _child;
}

void root::inject_before(entity _rsibling, entity _child) {
  assert(registry_.valid(_rsibling));
  auto &sibling_neighbors = assert_get<neighbors>(_rsibling);
  auto &parent_container  = assert_get<container>(sibling_neighbors.parent_);
  auto &child_neighbors   = registry_.get_or_emplace<neighbors>(_child);

  child_neighbors.parent_ = sibling_neighbors.parent_;
  child_neighbors.prev_ = sibling_neighbors.prev_;
  if (child_neighbors.prev_ != null)
    assert_get<neighbors>(child_neighbors.prev_).next_ = _child;
  child_neighbors.next_ = _rsibling;
  sibling_neighbors.prev_ = _child;

  if (parent_container.first_ == _rsibling)
    parent_container.first_ = _child;
}

struct widget {};

struct rect {};

void root::make_rect(entity _e, const render2d::box_params &_params) {
  assert(!registry_.any_of<widget>(_e));
  registry_.emplace<widget>(_e);
  registry_.emplace<rect>(_e);
  registry_.emplace<dirty>(_e);
  registry_.emplace<bounds>(_e);
  registry_.emplace<boxes>(_e, boxes_renderer_.allocate(1));
  registry_.emplace<layout_handler>(_e, [this, _params, _e](bbox_px _l) {
    auto copy = _params;
    copy.bbox_ = _l;
    auto &b = ui::assert_get<boxes>(registry_, _e);
    auto updator = b.update();
    render2d::set(updator[0], copy);
  });
}

void root::layout_container(entity _e, const container &_c, bbox_px &_bbox) {
  bbox_px copy = _bbox;
  entity child = _c.first_;
  while(child != null) {
    const auto &n = assert_get<neighbors>(child);
    layout(child, copy);
    child = n.next_;
  }
}

bbox_px root::layout_widget(entity _e, bbox_px &_bbox) {
  bbox_px cutted{0_px};
  if (auto *rl = registry_.try_get<rectcut_length>(_e))
    cutted = _bbox.cut(rl->side_, rl->length_);
  else if (auto *rr = registry_.try_get<rectcut_ratio>(_e))
    cutted = _bbox.cut(rr->side_, rr->ratio_);

  registry_.replace<bounds>(_e, cutted);
  if (auto *l = registry_.try_get<layout_handler>(_e))
    (*l)(cutted);
  return cutted;
}

void root::layout(entity _e, bbox_px &_bbox) {
  HUT_PROFILE_FUN(PLAYOUT, (ENTT_ID_TYPE)_e);
  if (auto *c = registry_.try_get<container>(_e)) {
    bbox_px size = layout_widget(_e, _bbox);
    layout_container(_e, *c, size);
  }
  else {
    layout_widget(_e, _bbox);
  }
}

void root::invalidate() {
  bbox_px bounds = {0, 0, parent_.size()};
  layout(entity_, bounds);
  parent_.invalidate(true);
}

}  //namespace hut::ui
