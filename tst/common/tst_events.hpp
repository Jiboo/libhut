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

void install_esc_close(window &_win) {
  _win.on_key.connect([&](keycode _kcode, keysym _ksym, bool _down) {
    if (_ksym == KSYM_ESC && !_down)
      _win.close();
    return false;
  });
}

void install_resizable_movable(window &_win) {
  _win.on_mouse.connect([&](u8 _button, mouse_event_type _type, vec2 _coords) {
    constexpr float resize_border_threshold = 20;

    edge coords_edge;
    if (_coords.x < resize_border_threshold)
      coords_edge |= LEFT;
    if (_coords.y < resize_border_threshold)
      coords_edge |= TOP;
    if (float(_win.size().x) - _coords.x < resize_border_threshold)
      coords_edge |= RIGHT;
    if (float(_win.size().y) - _coords.y < resize_border_threshold)
      coords_edge |= BOTTOM;
    _win.cursor(edge_cursor(coords_edge));

    static bool clicked = false;
    if (_type == MDOWN && _button == 1) clicked = true;
    else if (_type == MUP && _button == 1)
      clicked = false;
    else if (_type == MMOVE && clicked) {
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
  _win.on_resize.connect([_ubo](const uvec2 &_size) {
    mat4 proj = ortho<float>(0.f, float(_size.x), 0.f, float(_size.y));
    _ubo->set_subone(0, offsetof(TUBO, proj_), sizeof(mat4), &proj);
    return false;
  });
}

void install_continuous_redraw(display &_display, window &_win) {
  _win.on_frame.connect([&](display::duration _dt) {
    _display.post([&](auto) {
      _win.invalidate(true);
    });
    return false;
  });
}

void install_test_events(display &_display, window &_win) {
  install_esc_close(_win);
  install_resizable_movable(_win);
  install_continuous_redraw(_display, _win);
}

template<typename TUBO>
void install_test_events(display &_display, window &_win, shared_buffer_suballoc<TUBO> _ubo) {
  install_test_events(_display, _win);
  install_resizable_ubo<TUBO>(_win, _ubo);
}

}  // namespace hut
