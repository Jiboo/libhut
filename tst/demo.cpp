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
#include "hut/window.hpp"

using namespace std;
using namespace chrono_literals;
using namespace hut;

int main(int argc, char **argv) {
  auto start = display::clock::now();
  display d("testbed");

  d.post([](auto) { cout << "single job" << endl; });
  d.post_delayed([](auto) { cout << "single job, delayed" << endl; }, 5s);

  window w(d);
  auto pipeline = make_unique<colored_triangle_list>(w);

  auto byte_size = 32 * 1024;
  buffer b(d, byte_size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
           (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT));

  auto ubo = b.allocate<colored_triangle_list::ubo>();
  auto vertices = b.allocate<colored_triangle_list::vertex>(4);
  auto indices = b.allocate<uint16_t>(6);

  pipeline->set_ubo(ubo);
  vertices->set(std::initializer_list<colored_triangle_list::vertex>{
      {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
      {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}});
  indices->set(std::initializer_list<uint16_t>{0, 1, 2, 2, 3, 0});

  w.title("testbed");

  w.on_draw.connect([&pipeline, &vertices, &indices](VkCommandBuffer _buffer,
                                                     const glm::uvec2 &_size) {
    // cout << "drawing" << endl;
    pipeline->draw(_buffer, _size, vertices, indices);
    return false;
  });

  size_t fps = 0;
  display::time_point last_infos = display::clock::now();

  w.on_frame.connect([&w, &ubo, &last_infos, &fps](glm::uvec2 _size,
                                                   display::duration _delta) {
    w.invalidate(glm::uvec4{glm::uvec2{0, 0}, _size});

    static auto startTime = display::clock::now();

    auto currentTime = display::clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(
                     currentTime - startTime)
                     .count() /
                 1000.0f;

    colored_triangle_list::ubo new_ubo;
    new_ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f),
                                glm::vec3(0.0f, 0.0f, 1.0f));
    new_ubo.view =
        glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    new_ubo.proj = glm::perspective(
        glm::radians(45.0f), w.size().x / (float)w.size().y, 0.1f, 10.0f);
    new_ubo.proj[1][1] *= -1;

    ubo->set({new_ubo});

    fps++;

    if (currentTime - last_infos > 1s) {
      cout << "fps: " << fps << endl;
      fps = 0;
      last_infos = currentTime;
    }

    return false;
  });

  w.on_resize.connect([](const glm::uvec2 &_size) {
    // cout << "resize " << glm::to_string(_size) << endl;
    return false;
  });

  w.on_expose.connect([](const glm::uvec4 &_bounds) {
    // cout << "expose " << glm::to_string(_bounds) << endl;
    return false;
  });

  w.on_keysym.connect([&w](char32_t c, bool _press) {
    cout << "key " << _press << '\t' << c << '\t' << window::is_cursor_key(c)
         << window::is_function_key(c) << window::is_keypad_key(c)
         << window::is_modifier_key(c) << '\t' << window::name_key(c) << endl;
    return true;
  });

  w.on_mouse.connect(
      [](uint8_t _button, mouse_event_type _type, glm::uvec2 _pos) {
        if (_type != MMOVE)
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

  cout << "init took "
       << chrono::duration_cast<chrono::milliseconds>(display::clock::now() -
                                                      start)
              .count()
       << endl;

  return d.dispatch();
}
