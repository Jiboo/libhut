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

#include <iomanip>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>

#include "hut/display.hpp"
#include "hut/drawables/rgb.hpp"
#include "hut/drawables/rgba.hpp"
#include "hut/drawables/tex.hpp"
#include "hut/drawables/rgb_tex.hpp"
#include "hut/drawables/rgba_tex.hpp"
#include "hut/drawables/rgba_mask.hpp"
#include "hut/font.hpp"
#include "hut/window.hpp"

#include "demo_ttf.hpp"
#include "demo_png.hpp"

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace hut;

void dump_timer(display::clock::time_point _ref, const std::string &_msg) {
  cout << "[" << duration<double, milli>(display::clock::now() - _ref).count() << "] " << _msg << endl;
}

int main(int argc, char **argv) {
  auto start = display::clock::now();
  cout << fixed << setprecision(1);

  display d("testbed");
  dump_timer(start, "initialized display");

  d.post([start](auto) { dump_timer(start, "single job"); });
  d.post_delayed([start](auto) { dump_timer(start, "single job, delayed"); }, 5s);

  window w(d);
  w.clear_color({1, 0, 1, 1});
  w.title("testbed");
  dump_timer(start, "initialized window");

  auto byte_size = 1024 * 1024;
  buffer b(d, byte_size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
           (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
              | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
              | VK_BUFFER_USAGE_INDEX_BUFFER_BIT));
  dump_timer(start, "initialized buffer");

  auto rgb_pipeline = make_unique<rgb>(w);
  auto rgba_pipeline = make_unique<rgba>(w);
  auto tex_pipeline = make_unique<tex>(w);
  auto rgbt_pipeline = make_unique<rgb_tex>(w);
  auto rgbat_pipeline = make_unique<rgba_tex>(w);
  auto text_pipeline = make_unique<rgba_mask>(w);
  auto atlas_debug_pipeline = make_unique<rgba_mask>(w);
  dump_timer(start, "initialized pipelines");

  auto rgb_ubo = b.allocate<rgb::ubo>();
  auto rgba_ubo = b.allocate<rgba::ubo>();
  auto tex_ubo = b.allocate<tex::ubo>();
  auto rgbt_ubo = b.allocate<rgb_tex::ubo>();
  auto rgbat_ubo = b.allocate<rgba_tex::ubo>();
  auto text_ubo = b.allocate<rgba_mask::ubo>();
  auto atlas_debug_ubo = b.allocate<rgba_mask::ubo>();

  auto rgb_vertices = b.allocate<rgb::vertex>(4);
  auto rgba_vertices = b.allocate<rgba::vertex>(4);
  auto tex_vertices = b.allocate<tex::vertex>(4);
  auto rgbt_vertices = b.allocate<rgb_tex::vertex>(4);
  auto rgbat_vertices = b.allocate<rgba_tex::vertex>(4);
  auto atlas_debug_vertices = b.allocate<rgba_mask::vertex>(4);

  auto indices = b.allocate<uint16_t>(6);
  dump_timer(start, "allocated buffers");

  indices->set(std::initializer_list<uint16_t>{0, 1, 2, 2, 3, 0});
  rgb_vertices->set(std::initializer_list<rgb::vertex>{{{0,   0},   {1, 0, 0}},
                                                       {{100, 0},   {0, 1, 0}},
                                                       {{100, 100}, {1, 1, 1}},
                                                       {{0,   100}, {0, 0, 1}}});
  rgba_vertices->set(std::initializer_list<rgba::vertex>{{{0,   0},   {1, 0, 0, 0.5f}},
                                                         {{100, 0},   {0, 1, 0, 0.5f}},
                                                         {{100, 100}, {1, 1, 1, 0.5f}},
                                                         {{0,   100}, {0, 0, 1, 0.5f}}});
  tex_vertices->set(std::initializer_list<tex::vertex>{{{0, 0}, {0, 0}},
                                                       {{1, 0}, {1, 0}},
                                                       {{1, 1}, {1, 1}},
                                                       {{0, 1}, {0, 1}}});
  rgbt_vertices->set(std::initializer_list<rgb_tex::vertex>{{{0, 0}, {1, 0, 0}, {0, 0}},
                                                            {{1, 0}, {0, 1, 0}, {1, 0}},
                                                            {{1, 1}, {0, 0, 1}, {1, 1}},
                                                            {{0, 1}, {1, 1, 1}, {0, 1}}});
  rgbat_vertices->set(std::initializer_list<rgba_tex::vertex>{{{0, 0}, {1, 0, 0, 0.5f}, {0, 0}},
                                                              {{1, 0}, {0, 1, 0, 0.5f}, {1, 0}},
                                                              {{1, 1}, {0, 0, 1, 0.5f}, {1, 1}},
                                                              {{0, 1}, {1, 1, 1, 0.5f}, {0, 1}}});
  atlas_debug_vertices->set(std::initializer_list<rgba_mask::vertex>{{{0, 0}, {0, 0}},
                                                                     {{1, 0}, {1, 0}},
                                                                     {{1, 1}, {1, 1}},
                                                                     {{0, 1}, {0, 1}}});

  dump_timer(start, "copied data");

  rgb_pipeline->bind(rgb_ubo);
  rgba_pipeline->bind(rgba_ubo);

  shared_image texture;
  dump_timer(start, "initialized image");

  shared_font ttf;
  dump_timer(start, "initialized font");

  // filled later by shaper once we loaded and baked the font
  shared_ref<rgba_mask::vertex> text_vertices;
  shared_ref<uint16_t> text_indices;

  sampler samp(d);
  sampler samp_low(d, false);
  dump_timer(start, "initialized sampler");

  std::thread load_tex([&]() {
    texture = image::load_png(d, demo_png::tex1_png.data(), demo_png::tex1_png.size());
    dump_timer(start, "done loading texture");
    tex_pipeline->bind(tex_ubo, texture, samp);
    rgbt_pipeline->bind(rgbt_ubo, texture, samp);
    rgbat_pipeline->bind(rgbat_ubo, texture, samp);
    w.invalidate(true);  // will force to call on_draw on the next frame
    dump_timer(start, "bound tex pipelines");

    d.post([texture](auto) {
      uint8_t data[texture->pixel_size() * 100 * 100];
      memset(data, 0x00, sizeof(data));
      texture->update({100, 100, 200, 200}, data, texture->pixel_size() * 100);
    });
  });
  dump_timer(start, "started image load thread");

  shaper::result hello_world; // in this context to keep a ref on buffers
  std::thread load_ttf([&]() {
    ttf = make_shared<font>(d, demo_ttf::Roboto_Regular_ttf.data(), demo_ttf::Roboto_Regular_ttf.size(), glm::uvec2{256, 256});
    dump_timer(start, "done loading font");
    d.post([&](auto){
      text_pipeline->bind(text_ubo, ttf->atlas(), samp);
    });
    atlas_debug_pipeline->bind(atlas_debug_ubo, ttf->atlas(), samp_low);
    dump_timer(start, "bound font pipeline");
    shaper s;
    uint8_t size = 11;
    s.bake(b, hello_world, ttf, size, "The The quick brown fox, jumps over the lazy dog. fi VA", [&w]() {
      w.invalidate(true);  // will force to call on_draw when hello_world is ready (both buffs allocated)
    });
    dump_timer(start, "shaped hello world text");
  });
  dump_timer(start, "started font load thread");

  w.on_draw.connect(
      [&](VkCommandBuffer _buffer, const glm::uvec2 &_size) {
        rgb_pipeline->draw(_buffer, _size, rgb_vertices, indices);
        rgba_pipeline->draw(_buffer, _size, rgba_vertices, indices);
        if (texture) { // don't use the pipeline while we didn't loaded&bound the texture
          rgbt_pipeline->draw(_buffer, _size, rgbt_vertices, indices);
          tex_pipeline->draw(_buffer, _size, tex_vertices, indices);
          rgbat_pipeline->draw(_buffer, _size, rgbat_vertices, indices);
        }
        if (ttf) {
          atlas_debug_pipeline->draw(_buffer, _size, atlas_debug_vertices, indices);
        }
        if (hello_world) {
          text_pipeline->draw(_buffer, _size, hello_world.vertices_, hello_world.indices_);
        }
        return false;
      });

  size_t fps = 0;
  display::time_point last_infos = display::clock::now();

  w.on_frame.connect([&](glm::uvec2 _size, display::duration _delta) {
    w.invalidate(false);  // asks for a redraw, without recalling on_draw (shader animation, for example)

    static auto startTime = display::clock::now();

    auto currentTime = display::clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
    float ratio = _size.x / (float)_size.y;

    {
      rgb::ubo new_rgb_ubo;
      new_rgb_ubo.model_ = glm::mat4(1);
      new_rgb_ubo.view_ = glm::mat4(1);
      new_rgb_ubo.proj_ = glm::ortho<float>(0, _size.x, 0, _size.y);
      rgb_ubo->set({new_rgb_ubo});
    }

    {
      rgba::ubo new_rgba_ubo;
      new_rgba_ubo.model = glm::mat4(1);
      new_rgba_ubo.model = glm::translate(new_rgba_ubo.model, {0, 100.f, 0});
      new_rgba_ubo.view = glm::mat4(1);
      new_rgba_ubo.proj = glm::ortho<float>(0, _size.x, 0, _size.y);
      rgba_ubo->set({new_rgba_ubo});
    }

    {
      tex::ubo new_tex_ubo;
      new_tex_ubo.model_ = glm::mat4(1);
      new_tex_ubo.model_ = glm::translate(new_tex_ubo.model_, {0.f, 0.f, 0});
      new_tex_ubo.model_ = glm::scale(new_tex_ubo.model_, {389.f, 325.f, 1.f});
      new_tex_ubo.view_ = glm::mat4(1);
      new_tex_ubo.proj_ = glm::ortho<float>(0, _size.x, 0, _size.y);
      tex_ubo->set({new_tex_ubo});
    }

    {
      rgb_tex::ubo new_rgbt_ubo;
      new_rgbt_ubo.model_ = glm::mat4(1);
      new_rgbt_ubo.model_ = glm::translate(new_rgbt_ubo.model_, {100.f, 100.f, 0});
      new_rgbt_ubo.model_ = glm::translate(new_rgbt_ubo.model_, {+389.f / 2, +325.f / 2, 0});
      new_rgbt_ubo.model_ = glm::rotate(new_rgbt_ubo.model_, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      new_rgbt_ubo.model_ = glm::translate(new_rgbt_ubo.model_, {-389.f / 2, -325.f / 2, 0});
      new_rgbt_ubo.model_ = glm::scale(new_rgbt_ubo.model_, {389.f, 325.f, 1.f});
      new_rgbt_ubo.view_ = glm::mat4(1);
      new_rgbt_ubo.proj_ = glm::ortho<float>(0, _size.x, 0, _size.y);
      rgbt_ubo->set({new_rgbt_ubo});
    }

    {
      rgba_tex::ubo new_rgbat_ubo;
      new_rgbat_ubo.model_ = glm::mat4(1);
      new_rgbat_ubo.model_ = glm::translate(new_rgbat_ubo.model_, {200.f, 200.f, 0});
      new_rgbat_ubo.model_ = glm::translate(new_rgbat_ubo.model_, {+389.f / 2, +325.f / 2, 0});
      new_rgbat_ubo.model_ = glm::rotate(new_rgbat_ubo.model_, time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      new_rgbat_ubo.model_ = glm::translate(new_rgbat_ubo.model_, {-389.f / 2, -325.f / 2, 0});
      new_rgbat_ubo.model_ = glm::scale(new_rgbat_ubo.model_, {389.f, 325.f, 1.f});
      new_rgbat_ubo.view_ = glm::mat4(1);
      new_rgbat_ubo.proj_ = glm::ortho<float>(0, _size.x, 0, _size.y);
      rgbat_ubo->set({new_rgbat_ubo});
    }

    if (ttf) {
      auto atlas_size = ttf->atlas()->size();
      rgba_mask::ubo new_atlas_debug_ubo;
      new_atlas_debug_ubo.model_ = glm::mat4(1);
      new_atlas_debug_ubo.model_ = glm::translate(new_atlas_debug_ubo.model_, {800.f, 100.f, 0});
      new_atlas_debug_ubo.model_ = glm::scale(new_atlas_debug_ubo.model_, {atlas_size.x, atlas_size.y, 1.f});
      new_atlas_debug_ubo.model_ = glm::scale(new_atlas_debug_ubo.model_, {5.f, 5.f, 1.f});
      new_atlas_debug_ubo.color_ = glm::vec4(.9, 0, 0, 1);
      new_atlas_debug_ubo.view_ = glm::mat4(1);
      new_atlas_debug_ubo.proj_ = glm::ortho<float>(0, _size.x, 0, _size.y);
      atlas_debug_ubo->set({new_atlas_debug_ubo});
    }

    {
      rgba_mask::ubo new_text_ubo;
      new_text_ubo.model_ = glm::mat4(1);
      new_text_ubo.model_ = glm::translate(new_text_ubo.model_, {0.f, 200.f, 0});
      new_text_ubo.view_ = glm::mat4(1);
      new_text_ubo.proj_ = glm::ortho<float>(0, _size.x, 0, _size.y);
      new_text_ubo.color_ = glm::vec4(1, 1, 1, 1);
      text_ubo->set({new_text_ubo});
    }

    fps++;

    if (currentTime - last_infos > 1s) {
      cout << "fps: " << fps << endl;
      fps = 0;
      last_infos = currentTime;
    }

    return false;
  });

  w.on_resize.connect([](const glm::uvec2 &_size) {
    cout << "resize " << glm::to_string(_size) << endl;
    return false;
  });

  w.on_expose.connect([](const glm::uvec4 &_bounds) {
    // cout << "expose " << glm::to_string(_bounds) << endl;
    return false;
  });

  w.on_keysym.connect([](char32_t c, bool _press) {
    cout << "key " << _press << '\t' << c << '\t' << window::is_cursor_key(c) << window::is_function_key(c)
         << window::is_keypad_key(c) << window::is_modifier_key(c) << '\t' << window::name_key(c) << endl;
    return true;
  });

  w.on_mouse.connect([](uint8_t _button, mouse_event_type _type, glm::uvec2 _pos) {
    if (_type != MMOVE)
      cout << "mouse " << to_string(_button) << ", type: " << _type << ", pos: " << glm::to_string(_pos) << endl;
    return true;
  });

  /*w.on_close.connect([&w, &d, start] {
    dump_timer(start, "caught close...");
    d.post_delayed([&w, start](auto) {
      dump_timer(start, "closing...");
      w.close();
    }, 5s);
    return true;
  });*/

  w.on_focus.connect([start]() {
    dump_timer(start, "focused");
    return false;
  });

  w.on_blur.connect([start]() {
    dump_timer(start, "blured");
    return false;
  });

  w.visible(true);

  dump_timer(start, "finished callback setup");
  auto result = d.dispatch();
  load_tex.join();
  load_ttf.join();
  dump_timer(start, "done.");
  return result;
}
