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

#include "hut/pipeline.hpp"
#include "hut/window.hpp"

namespace hut {

void install_resizable_movable(window &_win) {
  _win.on_mouse_.connect([&](u8 _button, mouse_event_type _type, vec2 _coords) {
    constexpr float RESIZE_BORDER_THRESHOLD = 20;

    edge coords_edge;
    if (_coords.x < RESIZE_BORDER_THRESHOLD)
      coords_edge |= LEFT;
    if (_coords.y < RESIZE_BORDER_THRESHOLD)
      coords_edge |= TOP;
    if (float(_win.size().x) - _coords.x < RESIZE_BORDER_THRESHOLD)
      coords_edge |= RIGHT;
    if (float(_win.size().y) - _coords.y < RESIZE_BORDER_THRESHOLD)
      coords_edge |= BOTTOM;
    _win.cursor(edge_cursor(coords_edge));

    static bool s_clicked = false;
    if (_type == MDOWN && _button == 1)
      s_clicked = true;
    else if (_type == MUP && _button == 1)
      s_clicked = false;
    else if (_type == MMOVE && s_clicked) {
      if (!coords_edge)
        _win.interactive_move();
      else
        _win.interactive_resize(coords_edge);
      return true;
    }
    return false;
  });
}

template<typename TUBO>
void install_resizable_ubo(window &_win, shared_buffer_suballoc<TUBO> _ubo) {
  _win.on_resize_.connect([_ubo](const u16vec2_px &_size, u32 _scale) {
    mat4 proj = ortho<float>(0.f, float(_size.x * _scale), 0.f, float(_size.y * _scale));
    _ubo->set_subone(0, offsetof(TUBO, proj_), sizeof(mat4), &proj);

    float dpi_scale{static_cast<float>(_scale)};
    _ubo->set_subone(0, offsetof(TUBO, dpi_scale_), sizeof(dpi_scale), &dpi_scale);
    return false;
  });
}

void install_test_events(display &_display, window &_win) {
  install_resizable_movable(_win);
}

template<typename TUBO>
void install_test_events(display &_display, window &_win, shared_buffer_suballoc<TUBO> _ubo) {
  install_test_events(_display, _win);
  install_resizable_ubo<TUBO>(_win, _ubo);
}

}  // namespace hut
