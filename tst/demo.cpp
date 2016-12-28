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

#include <iostream>

#include <glm/ext.hpp>

#include "hut/display.hpp"
#include "hut/drawables/colored_triangle_list.hpp"
#include "hut/drawables/noinput.hpp"
#include "hut/window.hpp"

using namespace std;
using namespace chrono_literals;

int main(int argc, char **argv) {
  auto start = chrono::steady_clock::now();
  hut::display d("testbed");
  hut::window w(d);
  unique_ptr<hut::noinput> noinput = std::make_unique<hut::noinput>(w);
  unique_ptr<hut::colored_triangle_list> colored_triangle_list =
      std::make_unique<hut::colored_triangle_list>(w);

  auto byte_size = sizeof(hut::colored_triangle_list::vertex) * 100;
  hut::buffer b(d, byte_size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));

  auto buf_ref = b.allocate<hut::colored_triangle_list::vertex, 3>();
  auto buf_ref2 = b.allocate<hut::colored_triangle_list::vertex, 3>();
  auto buf_ref3 = b.allocate<hut::colored_triangle_list::vertex, 6>();

  buf_ref->set(std::initializer_list<hut::colored_triangle_list::vertex>{
      {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
      {{0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
      {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
  });

  buf_ref2->set(std::initializer_list<hut::colored_triangle_list::vertex>{
      {{0.2f, -0.3f}, {1.0f, 0.0f, 0.0f}},
      {{0.7f, 0.7f}, {1.0f, 0.0f, 0.0f}},
      {{-0.3f, 0.7f}, {1.0f, 0.0f, 0.0f}},
  });

  buf_ref3->set(std::initializer_list<hut::colored_triangle_list::vertex>{
      {{-0.2f, -0.7f}, {0.0f, 1.0f, 0.0f}},
      {{0.7f, 0.7f}, {0.0f, 1.0f, 0.0f}},
      {{-0.7f, 0.7f}, {00.0f, 1.0f, 0.0f}},
      {{-0.8f, -0.8f}, {0.0f, 1.0f, 0.0f}},
      {{0.7f, 0.7f}, {0.0f, 1.0f, 0.0f}},
      {{-0.7f, 0.7f}, {00.0f, 1.0f, 0.0f}},
  });

  w.title("testbed");

  w.on_draw.connect(
      [&noinput, &colored_triangle_list, &buf_ref, &buf_ref2, &buf_ref3](
          VkCommandBuffer _buffer, const glm::uvec2 &_size) {
        // cout << "drawing" << endl;
        colored_triangle_list->draw(_buffer, _size, buf_ref);
        colored_triangle_list->draw(_buffer, _size, buf_ref2);
        colored_triangle_list->draw(_buffer, _size, buf_ref3);
        noinput->draw(_buffer, _size);
        return false;
      });

  w.on_resize.connect([&noinput, &w](const glm::uvec2 &_size) {
    // cout << "resize " << glm::to_string(_size) << endl;
    return false;
  });

  w.on_expose.connect([](const glm::uvec4 &_bounds) {
    // cout << "expose " << glm::to_string(_bounds) << endl;
    return false;
  });

  w.on_keysym.connect([&w](char32_t c, bool _press) {
    cout << "key " << _press << '\t' << c << '\t'
         << hut::window::is_cursor_key(c) << hut::window::is_function_key(c)
         << hut::window::is_keypad_key(c) << hut::window::is_modifier_key(c)
         << '\t' << hut::window::name_key(c) << endl;
    return true;
  });

  w.on_mouse.connect(
      [](uint8_t _button, hut::mouse_event_type _type, glm::uvec2 _pos) {
        if (_type != hut::MMOVE)
          cout << "mouse " << to_string(_button) << ", type: " << _type
               << ", pos: " << glm::to_string(_pos) << endl;
        return true;
      });

  w.on_close.connect([&w, &d] {
    cout << "catching close" << endl;
    d.post_delayed([&w](auto) { w.close(); }, 5s);
    return true;
  });

  w.on_focus.connect([]() {
    // cout << "focused" << endl;
    return false;
  });

  w.on_blur.connect([]() {
    // cout << "blured" << endl;
    return false;
  });

  w.visible(true);

  using namespace chrono_literals;

  d.post([](auto) { cout << "single job" << endl; });

  d.post_delayed([](auto) { cout << "single job, delayed" << endl; }, 5s);

  using tp_t = hut::display::time_point;
  bool continue_profiling = false;
  size_t fps = 0;
  tp_t last_infos = chrono::steady_clock::now();

  function<void(tp_t)> anim_frame;
  anim_frame = [&d, &w, &anim_frame, &last_infos, &fps,
                &continue_profiling](tp_t _now) {
    if (continue_profiling) d.post(anim_frame);
    w.invalidate(glm::uvec4{glm::uvec2{0, 0}, w.size()});
    fps++;
    if (_now - last_infos > 1s) {
      cout << "fps: " << fps << endl;
      fps = 0;
      last_infos = _now;
    }
  };

  w.on_pause.connect([&continue_profiling]() {
    continue_profiling = false;
    return false;
  });
  w.on_resume.connect([&d, &anim_frame, &continue_profiling]() {
    continue_profiling = true;
    d.post(anim_frame);
    return false;
  });

  cout << "init took "
       << chrono::duration_cast<chrono::milliseconds>(
              chrono::steady_clock::now() - start)
              .count()
       << endl;

  return d.dispatch();
}
