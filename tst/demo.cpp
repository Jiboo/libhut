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

#include "hut/display.hpp"
#include "hut/drawable.hpp"
#include "hut/font.hpp"
#include "hut/window.hpp"

#include "demo_ttf.hpp"
#include "demo_png.hpp"

using namespace hut;

mat4 make_mat(vec2 _translate, vec3 _scale) {
  mat4 m(1);
  m = translate(m, vec3(_translate, 0));
  m = scale(m, _scale);
  return m;
}

mat4 make_mat(vec2 _translate, float _rot_angle, vec2 _rot_center, vec3 _scale) {
  mat4 m(1);
  m = translate(m, vec3(_translate, 0));
  m = translate(m, vec3(_rot_center, 0));
  m = rotate(m, _rot_angle, {0, 0, 1});
  m = translate(m, vec3(-_rot_center, 0));
  m = scale(m, _scale);
  return m;
}

int main(int, char **) {
  std::cout << std::fixed << std::setprecision(1);

  display d("testbed");

  window w(d);
  w.clear_color({1, 1, 1, 1});
  w.title("testbed");

  auto b = d.alloc_buffer(1024*1024);

  auto indices = b->allocate<uint16_t>(6);
  indices->set({0, 1, 2, 2, 3, 0});

  buffer_ubo default_ubo;
  default_ubo.proj_ = ortho<float>(0, w.size().x, 0, w.size().y);
  shared_ubo ubo = buffer_ubo::alloc(b, default_ubo);

  auto rgb_pipeline = std::make_unique<rgb>(w);
  auto rgb_instances = b->allocate<rgb::instance>(2);
  auto rgb_vertices = b->allocate<rgb::vertex>(4);
  rgb_vertices->set({
    rgb::vertex{{0,     0}, {1, 0, 0}},
    rgb::vertex{{100,   0}, {0, 1, 0}},
    rgb::vertex{{100, 100}, {1, 1, 1}},
    rgb::vertex{{0,   100}, {0, 0, 1}}
  });
  rgb_instances->set({
    rgb::instance{make_mat({  0, 100}, {1,1,1})},
    rgb::instance{make_mat({100,   0}, {1,1,1})}
  });
  rgb_pipeline->bind(0, ubo);

  auto rgba_pipeline = std::make_unique<rgba>(w);
  auto rgba_instances = b->allocate<rgba::instance>(2);
  auto rgba_vertices = b->allocate<rgba::vertex>(4);
  rgba_vertices->set({
    rgba::vertex{{0,     0}, {1, 0, 0, 0.25}},
    rgba::vertex{{100,   0}, {0, 1, 0, 0.25}},
    rgba::vertex{{100, 100}, {1, 1, 1, 0.25}},
    rgba::vertex{{0,   100}, {0, 0, 1, 0.25}}
  });
  rgba_instances->set({
    rgba::instance{make_mat({  0,   0}, {1,1,1})},
    rgba::instance{make_mat({100, 100}, {1,1,1})}
  });
  rgba_pipeline->bind(0, ubo);

  auto tex_pipeline = std::make_unique<tex>(w);
  tex_pipeline->alloc_next_descriptors(1);
  auto tex1_instances = b->allocate<tex::instance>(2);
  auto tex2_instances = b->allocate<tex::instance>(2);
  auto tex_vertices = b->allocate<tex::vertex>(4);
  tex_vertices->set({
    tex::vertex{{0, 0}, {0, 0}},
    tex::vertex{{1, 0}, {1, 0}},
    tex::vertex{{1, 1}, {1, 1}},
    tex::vertex{{0, 1}, {0, 1}}
  });

  auto tex_rgb_pipeline = std::make_unique<tex_rgb>(w);
  tex_rgb_pipeline->alloc_next_descriptors(1);
  auto tex1_rgb_instances = b->allocate<tex_rgb::instance>(2);
  auto tex2_rgb_instances = b->allocate<tex_rgb::instance>(2);
  auto tex_rgb_vertices = b->allocate<tex_rgb::vertex>(4);
  tex_rgb_vertices->set({
    tex_rgb::vertex{{0, 0}, {0, 0}, {1, 0, 0}},
    tex_rgb::vertex{{1, 0}, {1, 0}, {0, 1, 0}},
    tex_rgb::vertex{{1, 1}, {1, 1}, {1, 1, 1}},
    tex_rgb::vertex{{0, 1}, {0, 1}, {0, 0, 1}}
  });
  tex1_rgb_instances->set({
    tex_rgb::instance{make_mat({  0, 200}, {100,100,1})},
    tex_rgb::instance{make_mat({100, 300}, {100,100,1})},
  });
  tex2_rgb_instances->set({
    tex_rgb::instance{make_mat({100, 200}, {100,100,1})},
    tex_rgb::instance{make_mat({  0, 300}, {100,100,1})},
  });

  auto tex_rgba_pipeline = std::make_unique<tex_rgba>(w);
  tex_rgba_pipeline->alloc_next_descriptors(1);
  auto tex1_rgba_instances = b->allocate<tex_rgba::instance>(2);
  auto tex2_rgba_instances = b->allocate<tex_rgba::instance>(2);
  auto tex_rgba_vertices = b->allocate<tex_rgba::vertex>(4);
  tex_rgba_vertices->set({
    tex_rgba::vertex{{0, 0}, {0, 0}, {1, 0, 0, 0.25}},
    tex_rgba::vertex{{1, 0}, {1, 0}, {0, 1, 0, 0.25}},
    tex_rgba::vertex{{1, 1}, {1, 1}, {1, 1, 1, 0.25}},
    tex_rgba::vertex{{0, 1}, {0, 1}, {0, 0, 1, 0.25}}
  });
  tex1_rgba_instances->set({
    tex_rgba::instance{make_mat({  0, 400}, {100,100,1})},
    tex_rgba::instance{make_mat({100, 500}, {100,100,1})},
  });
  tex2_rgba_instances->set({
    tex_rgba::instance{make_mat({100, 400}, {100,100,1})},
    tex_rgba::instance{make_mat({  0, 500}, {100,100,1})},
  });

  auto tmask_pipeline = std::make_unique<tex_mask>(w);
  tmask_pipeline->alloc_next_descriptors(1);
  auto text_instances = b->allocate<tex_mask::instance>(2);
  auto icons_instances = b->allocate<tex_mask::instance>(2);
  text_instances->set({
    tex_mask::instance{make_mat({  1, 101}, {1,1,1}), {0, 0, 0, 1}},
    tex_mask::instance{make_mat({  0, 100}, {1,1,1}), {1, 1, 1, 1}},
  });
  icons_instances->set({
    tex_mask::instance{make_mat({  1, 201}, {1,1,1}), {0, 0, 0, 1}},
    tex_mask::instance{make_mat({  0, 200}, {1,1,1}), {1, 1, 1, 1}},
  });

  auto box_rgba_pipeline = std::make_unique<box_rgba>(w);
  auto box_rgba_vertices = b->allocate<box_rgba::vertex>(4);
  box_rgba_vertices->set({
    box_rgba::vertex{{  0,   0}, {1, 0, 0, 1}},
    box_rgba::vertex{{100,   0}, {0, 1, 0, 1}},
    box_rgba::vertex{{100, 100}, {1, 1, 1, 1}},
    box_rgba::vertex{{  0, 100}, {0, 0, 1, 1}}
  });
  auto box_rgba_instances = b->allocate<box_rgba::instance>(2);
  box_rgba_instances->set({
    box_rgba::instance{make_mat({300, 400}, {1,1,1}), {0, 2, 0.8, 8}, {0, 0, 0, 0.66}, {300, 400, 400, 500}},
    box_rgba::instance{make_mat({500, 400}, {1,1,1}), {8, 8, 50, 8}, {0, 0, 0, 0.66}, {500, 400, 600, 500}},
  });
  box_rgba_pipeline->bind(0, ubo);

  shared_image tex1, tex2;
  shared_font roboto, material_icons;
  shared_sampler samp = std::make_shared<sampler>(d);

  // in this context to keep a ref on buffers
  shaper::result hello_world;
  shaper::result icons_test;

  std::atomic_bool tex1_ready = false, tex2_ready = false;

  std::thread load_res([&]() {
    tex1 = image::load_png(d, demo_png::tex1_png.data(), demo_png::tex1_png.size());
    tex_pipeline->bind(0, ubo, tex1, samp);
    tex_rgb_pipeline->bind(0, ubo, tex1, samp);
    tex_rgba_pipeline->bind(0, ubo, tex1, samp);
    tex1_instances->set({
      tex::instance{make_mat({200,   0}, {tex1->size() / 2, 1})},
      tex::instance{make_mat({300, 100}, {tex1->size() / 2, 1})},
    });
    tex1_ready = true;
    w.invalidate(true);  // will force to call on_draw on the next frame

    uint8_t data[tex1->pixel_size() * 100 * 100];
    memset(data, 0x80, sizeof(data));
    tex1->update({100, 100, 200, 200}, data, tex1->pixel_size() * 100);

    tex2 = image::load_png(d, demo_png::tex2_png.data(), demo_png::tex2_png.size());
    tex_pipeline->bind(1, ubo, tex2, samp);
    tex_rgb_pipeline->bind(1, ubo, tex2, samp);
    tex_rgba_pipeline->bind(1, ubo, tex2, samp);
    tex2_instances->set({
      tex::instance{make_mat({300,   0}, {tex2->size() / 2, 1})},
      tex::instance{make_mat({400, 100}, {tex2->size() / 2, 1})},
    });
    tex2_ready = true;
    w.invalidate(true);  // will force to call on_draw on the next frame

    roboto = std::make_shared<font>(d, demo_ttf::Roboto_Regular_ttf.data(), demo_ttf::Roboto_Regular_ttf.size(), uvec2{256, 256}, true);
    material_icons = std::make_shared<font>(d, demo_ttf::MaterialIcons_Regular_ttf.data(), demo_ttf::MaterialIcons_Regular_ttf.size(), uvec2{256, 256}, false);
    tmask_pipeline->bind(0, ubo, roboto->atlas(), samp);
    tmask_pipeline->bind(1, ubo, material_icons->atlas(), samp);

    shaper s;
    s.bake(b, hello_world, roboto, 12, "The The quick brown fox, jumps over the lazy dog. fi VA");
    s.bake(b, icons_test, material_icons, 16, "\uE834\uE835\uE836\uE837");

    w.invalidate(true);
  });

  w.on_draw.connect([&](VkCommandBuffer _buffer, const uvec2 &_size) {
    rgb_pipeline->draw(_buffer, _size, 0, indices, rgb_instances, rgb_vertices);
    rgba_pipeline->draw(_buffer, _size, 0, indices, rgba_instances, rgba_vertices);
    box_rgba_pipeline->draw(_buffer, _size, 0, indices, box_rgba_instances, box_rgba_vertices);
    if (tex1_ready) {
      tex_pipeline->draw(_buffer, _size, 0, indices, tex1_instances, tex_vertices);
      tex_rgb_pipeline->draw(_buffer, _size, 0, indices, tex1_rgb_instances, tex_rgb_vertices);
      tex_rgba_pipeline->draw(_buffer, _size, 0, indices, tex1_rgba_instances, tex_rgba_vertices);
    }
    if (tex2_ready) {
      tex_pipeline->draw(_buffer, _size, 1, indices, tex2_instances, tex_vertices);
      tex_rgb_pipeline->draw(_buffer, _size, 1, indices, tex2_rgb_instances, tex_rgb_vertices);
      tex_rgba_pipeline->draw(_buffer, _size, 1, indices, tex2_rgba_instances, tex_rgba_vertices);
    }
    if (hello_world)
      tmask_pipeline->draw(_buffer, _size, 0, hello_world.indices_, text_instances, hello_world.vertices_);
    if (icons_test)
      tmask_pipeline->draw(_buffer, _size, 1, icons_test.indices_, icons_instances, icons_test.vertices_);
    return false;
  });

  w.on_frame.connect([&](uvec2 _size, display::duration _delta) {
    d.post([&w](auto){
      w.invalidate(false);  // asks for a redraw, without recalling on_draw (shader animation, for example)
    });

    static auto startTime = display::clock::now();

    auto currentTime = display::clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    float angle = radians(time * 10);
    if (tex1_ready) {
      tex1_instances->update(1, tex::instance{make_mat({300, 100}, angle, tex1->size()/4, {tex1->size()/2, 1})});
    }
    if (tex2_ready) {
      tex2_instances->update(1, tex::instance{make_mat({400, 100}, angle, tex2->size()/4, {tex2->size()/2, 1})});
    }

    float shadow_offset = (sin(time * 4) + 1) * 4;
    vec4 new_box_params = {0, shadow_offset, 16, 8};
    box_rgba_instances->update(0, offsetof(box_rgba::instance, params_), sizeof(new_box_params), &new_box_params);

    float border_radius = (sin(time * 4) + 1) * 20 + 10;
    new_box_params = {16, 16, border_radius, 8};
    box_rgba_instances->update(1, offsetof(box_rgba::instance, params_), sizeof(new_box_params), &new_box_params);

    float blink = (sin(time * 4) + 1) / 2;
    constexpr uint transparency_offset = offsetof(tex_mask::instance, color_) + 3 * sizeof(float);
    icons_instances->update(1, transparency_offset, sizeof(float), &blink);

    return false;
  });

  w.on_resize.connect([&](const uvec2 &_size) {
    //cout << "resize " << to_string(_size) << endl;
    buffer_ubo new_ubo;
    new_ubo.proj_ = ortho<float>(0, _size.x, 0, _size.y);

    ubo->set(&new_ubo);
    return false;
  });

  w.on_expose.connect([](const uvec4 &_bounds) {
    //std::cout << "expose " << to_string(_bounds) << std::endl;
    return false;
  });

  w.on_key.connect([](keysym c, bool _press) {
    std::cout << "keysym " << _press << '\t' << window::name_key(c) << std::endl;
    return true;
  });

  w.on_char.connect([](char32_t c) {
    std::cout << "char " << to_utf8(c) << std::endl;
    return true;
  });

  w.on_mouse.connect([](uint8_t _button, mouse_event_type _type, uvec2 _pos) {
    if (_type != MMOVE)
      std::cout << "mouse " << std::to_string(_button) << ", type: " << _type << ", pos: " << to_string(_pos) << std::endl;
    return true;
  });

  w.on_close.connect([] {
    return false;
  });

  w.on_focus.connect([]() {
    std::cout << "focused" << std::endl;
    return false;
  });

  w.on_blur.connect([]() {
    std::cout << "blurred" << std::endl;
    return false;
  });

  w.on_pause.connect([]() {
    std::cout << "paused" << std::endl;
    return false;
  });

  w.on_resume.connect([]() {
    std::cout << "resumed" << std::endl;
    return false;
  });

  w.visible(true);

  auto result = d.dispatch();
  load_res.join();
  std::cout << "done!" << std::endl;
  return result;
}
