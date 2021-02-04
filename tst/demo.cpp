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
#include "hut/pipeline.hpp"
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

  display d("hut demo");

  window w(d);
  w.clear_color({1, 1, 1, 1});
  w.title("hut demo win1");

  window w2(d);
  w2.clear_color({0, 0, 1, 1});
  w2.title("hut demo win2");

  shared_sampler samp = std::make_shared<sampler>(d);

  auto b = d.alloc_buffer(1024*1024);

  auto indices = b->allocate<uint16_t>(6);
  indices->set({0, 1, 2, 2, 1, 3});

  proj_ubo default_ubo;
  default_ubo.proj_ = ortho<float>(0, w.size().x, 0, w.size().y);
  shared_ref<proj_ubo> ubo = d.alloc_ubo(b, default_ubo);

  auto rgb_pipeline = std::make_unique<rgb>(w);
  auto rgb_instances = b->allocate<rgb::instance>(2);
  auto rgb_vertices = b->allocate<rgb::vertex>(4);
  rgb_vertices->set({
    rgb::vertex{{0,     0}, {1, 0, 0}},
    rgb::vertex{{0,   100}, {0, 0, 1}},
    rgb::vertex{{100,   0}, {0, 1, 0}},
    rgb::vertex{{100, 100}, {1, 1, 1}},
  });
  rgb_instances->set({
    rgb::instance{make_mat({  0, 100}, {1,1,1})},
    rgb::instance{make_mat({100,   0}, {1,1,1})}
  });
  rgb_pipeline->write(0, ubo);

  auto rgb_pipeline2 = std::make_unique<rgb>(w2);
  rgb_pipeline2->write(0, ubo);

  auto rgba_pipeline = std::make_unique<rgba>(w);
  auto rgba_instances = b->allocate<rgba::instance>(2);
  auto rgba_vertices = b->allocate<rgba::vertex>(4);
  rgba_vertices->set({
    rgba::vertex{{0,     0}, {1, 0, 0, 0.25}},
    rgba::vertex{{0,   100}, {0, 0, 1, 0.25}},
    rgba::vertex{{100,   0}, {0, 1, 0, 0.25}},
    rgba::vertex{{100, 100}, {1, 1, 1, 0.25}},
  });
  rgba_instances->set({
    rgba::instance{make_mat({  0,   0}, {1,1,1})},
    rgba::instance{make_mat({100, 100}, {1,1,1})}
  });
  rgba_pipeline->write(0, ubo);

  auto tex_pipeline = std::make_unique<tex>(w);
  tex_pipeline->alloc_next_descriptors(2);
  auto tex1_instances = b->allocate<tex::instance>(2);
  auto tex2_instances = b->allocate<tex::instance>(2);
  auto tex_vertices = b->allocate<tex::vertex>(4);
  tex_vertices->set({
    tex::vertex{{0, 0}, {0, 0}},
    tex::vertex{{0, 1}, {0, 1}},
    tex::vertex{{1, 0}, {1, 0}},
    tex::vertex{{1, 1}, {1, 1}},
  });

  auto tex_rgb_pipeline = std::make_unique<tex_rgb>(w);
  tex_rgb_pipeline->alloc_next_descriptors(1);
  auto tex1_rgb_instances = b->allocate<tex_rgb::instance>(2);
  auto tex2_rgb_instances = b->allocate<tex_rgb::instance>(2);
  auto tex_rgb_vertices = b->allocate<tex_rgb::vertex>(4);
  tex_rgb_vertices->set({
    tex_rgb::vertex{{0, 0}, {0, 0}, {1, 0, 0}},
    tex_rgb::vertex{{0, 1}, {0, 1}, {0, 0, 1}},
    tex_rgb::vertex{{1, 0}, {1, 0}, {0, 1, 0}},
    tex_rgb::vertex{{1, 1}, {1, 1}, {1, 1, 1}},
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
    tex_rgba::vertex{{0, 1}, {0, 1}, {0, 0, 1, 0.25}},
    tex_rgba::vertex{{1, 0}, {1, 0}, {0, 1, 0, 0.25}},
    tex_rgba::vertex{{1, 1}, {1, 1}, {1, 1, 1, 0.25}},
  });
  tex1_rgba_instances->set({
    tex_rgba::instance{make_mat({  0, 400}, {100,100,1})},
    tex_rgba::instance{make_mat({100, 500}, {100,100,1})},
  });
  tex2_rgba_instances->set({
    tex_rgba::instance{make_mat({100, 400}, {100,100,1})},
    tex_rgba::instance{make_mat({  0, 500}, {100,100,1})},
  });

  shared_atlas r8_atlas = std::make_shared<atlas>(d, VK_FORMAT_R8_UNORM);
  auto tmask_pipeline = std::make_unique<tex_mask>(w);
  tmask_pipeline->write(0, ubo, r8_atlas->image(), samp);

  auto text_instances = b->allocate<tex_mask::instance>(2);
  auto icons_instances = b->allocate<tex_mask::instance>(2);
  auto fps_instances = b->allocate<tex_mask::instance>(2);
  auto atlas_instances = b->allocate<tex_mask::instance>(1);
  auto atlas_vertices = b->allocate<tex_mask::vertex>(4);
  text_instances->set({
    tex_mask::instance{make_mat({  1, 101}, {1,1,1}), {0, 0, 0, 1}},
    tex_mask::instance{make_mat({  0, 100}, {1,1,1}), {1, 1, 1, 1}},
  });
  icons_instances->set({
    tex_mask::instance{make_mat({  1, 201}, {1,1,1}), {0, 0, 0, 1}},
    tex_mask::instance{make_mat({  0, 200}, {1,1,1}), {1, 1, 1, 1}},
  });
  fps_instances->set({
    tex_mask::instance{make_mat({  6, 17}, {1,1,1}), {0, 0, 0, 1}},
    tex_mask::instance{make_mat({  5, 16}, {1,1,1}), {1, 1, 1, 1}},
  });
  atlas_vertices->set({
    tex_mask::vertex{{0, 0}, {0, 0}},
    tex_mask::vertex{{0, 1}, {0, 1}},
    tex_mask::vertex{{1, 0}, {1, 0}},
    tex_mask::vertex{{1, 1}, {1, 1}},
  });
  atlas_instances->set(tex_mask::instance{make_mat({200, 0}, {r8_atlas->image()->size(), 1}), {1, 0, 0, 1}});

  auto box_rgba_pipeline = std::make_unique<box_rgba>(w);
  auto box_rgba_vertices = b->allocate<box_rgba::vertex>(4);
  auto shadow_vertices = b->allocate<box_rgba::vertex>(4);
  box_rgba_vertices->set({
    box_rgba::vertex{{  0,   0}, {1, 0, 0, 1}},
    box_rgba::vertex{{  0, 100}, {0, 0, 1, 1}},
    box_rgba::vertex{{100,   0}, {0, 1, 0, 1}},
    box_rgba::vertex{{100, 100}, {1, 1, 1, 1}},
  });
  shadow_vertices->set({
     box_rgba::vertex{{  0,   0}, {0, 0, 0, 1}},
     box_rgba::vertex{{  0, 100}, {0, 0, 0, 1}},
     box_rgba::vertex{{100,   0}, {0, 0, 0, 1}},
     box_rgba::vertex{{100, 100}, {0, 0, 0, 1}},
  });
  auto box_rgba_instances = b->allocate<box_rgba::instance>(1);
  auto shadow_instances = b->allocate<box_rgba::instance>(1);
  box_rgba_instances->set({
    box_rgba::instance{make_mat({300, 400}, {1,1,1}), {8, 8}, {300, 400, 400, 500}},
  });
  shadow_instances->set({
    box_rgba::instance{make_mat({300, 400}, {1,1,1}), {8, 8}, {300, 400, 400, 500}},
  });
  box_rgba_pipeline->write(0, ubo);

  shared_image tex1, tex2;
  shared_font roboto, material_icons;

  shaper s;
  // in this context to keep a ref on buffers
  shaper::result hello_world;
  shaper::result fps_counter;
  shaper::result icons_test;

  std::atomic_bool tex1_ready = false, tex2_ready = false;

  std::thread load_res([&]() {
    tex1 = image::load_png(d, demo_png::tex1_png);
    tex_pipeline->write(0, ubo, tex1, samp);
    tex_rgb_pipeline->write(0, ubo, tex1, samp);
    tex_rgba_pipeline->write(0, ubo, tex1, samp);
    tex1_instances->set({
      tex::instance{make_mat({200,   0}, {tex1->size() / 2, 1})},
      tex::instance{make_mat({300, 100}, {tex1->size() / 2, 1})},
    });
    tex1_ready = true;
    w.invalidate(true);  // will force to call on_draw on the next frame

    uint8_t data[tex1->pixel_size() * 100 * 100];
    uint32_t *pix_data = (uint32_t*)data;
    for (int x = 0; x < 100; x++)
      for (int y = 0; y < 100; y++)
        pix_data[y*100+x] = 0x80808080;
    tex1->update({100, 100, 200, 200}, data, tex1->pixel_size() * 100);
    w.invalidate(true);  // will force to call on_draw on the next frame

    tex2 = image::load_png(d, demo_png::tex2_png);
    tex_pipeline->write(1, ubo, tex2, samp);
    tex_rgb_pipeline->write(1, ubo, tex2, samp);
    tex_rgba_pipeline->write(1, ubo, tex2, samp);
    tex2_instances->set({
      tex::instance{make_mat({300,   0}, {tex2->size() / 2, 1})},
      tex::instance{make_mat({400, 100}, {tex2->size() / 2, 1})},
    });
    tex2_ready = true;
    w.invalidate(true);  // will force to call on_draw on the next frame

    roboto = std::make_shared<font>(d, demo_ttf::Roboto_Regular_ttf.data(), demo_ttf::Roboto_Regular_ttf.size(), r8_atlas, true);
    material_icons = std::make_shared<font>(d, demo_ttf::MaterialIcons_Regular_ttf.data(), demo_ttf::MaterialIcons_Regular_ttf.size(), r8_atlas, false);
    s.bake(b, icons_test, material_icons, 16, "\uE834\uE835\uE836\uE837");
    w.invalidate(true);  // will force to call on_draw on the next frame
  });

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
    tmask_pipeline->draw(_buffer, 0, indices, atlas_instances, atlas_vertices);

    rgb_pipeline->draw(_buffer, 0, indices, rgb_instances, rgb_vertices);
    rgba_pipeline->draw(_buffer, 0, indices, rgba_instances, rgba_vertices);
    box_rgba_pipeline->draw(_buffer, 0, indices, shadow_instances, shadow_vertices);
    box_rgba_pipeline->draw(_buffer, 0, indices, box_rgba_instances, box_rgba_vertices);
    if (tex1_ready) {
      tex_pipeline->draw(_buffer, 0, indices, tex1_instances, tex_vertices);
      tex_rgb_pipeline->draw(_buffer, 0, indices, tex1_rgb_instances, tex_rgb_vertices);
      tex_rgba_pipeline->draw(_buffer, 0, indices, tex1_rgba_instances, tex_rgba_vertices);
    }
    if (tex2_ready) {
      tex_pipeline->draw(_buffer, 1, indices, tex2_instances, tex_vertices);
      tex_rgb_pipeline->draw(_buffer, 1, indices, tex2_rgb_instances, tex_rgb_vertices);
      tex_rgba_pipeline->draw(_buffer, 1, indices, tex2_rgba_instances, tex_rgba_vertices);
    }

    if (hello_world)
      tmask_pipeline->draw(_buffer, 0, hello_world.indices_, hello_world.indices_count_, text_instances, hello_world.vertices_);
    if (icons_test)
      tmask_pipeline->draw(_buffer, 0, icons_test.indices_, icons_test.indices_count_, icons_instances, icons_test.vertices_);
    if (fps_counter)
      tmask_pipeline->draw(_buffer, 0, fps_counter.indices_, fps_counter.indices_count_, fps_instances, fps_counter.vertices_);

    return false;
  });

  w2.on_draw.connect([&](VkCommandBuffer _buffer) {
    rgb_pipeline2->draw(_buffer, 0, indices, rgb_instances, rgb_vertices);
    return false;
  });

  w.on_frame.connect([&](display::duration) {
    d.post([&w](auto){
      w.invalidate(false);  // asks for another frame without a redraw to continue animations and fps count
    });

    static auto startTime = display::clock::now();

    auto currentTime = display::clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    float angle = radians(time * 10);
    if (tex1_ready) {
      tex1_instances->update_one(1, tex::instance{make_mat({300, 100}, angle, tex1->size()/4, {tex1->size()/2, 1})});
    }
    if (tex2_ready) {
      tex2_instances->update_one(1, tex::instance{make_mat({400, 100}, angle, tex2->size()/4, {tex2->size()/2, 1})});
    }

    float border_radius = (sin(time * 4) + 1) * 20 + 1;
    vec2 new_box_params = {border_radius, 1};
    box_rgba_instances->update_subone(0, offsetof(box_rgba::instance, params_), sizeof(new_box_params), &new_box_params);

    new_box_params[1] = (sin(time * 4) + 1) * 10 + 1;
    shadow_instances->update_subone(0, offsetof(box_rgba::instance, params_), sizeof(new_box_params), &new_box_params);

    float blink = (sin(time * 4) + 1) / 2;
    constexpr uint transparency_offset = offsetof(tex_mask::instance, color_) + 3 * sizeof(float);
    icons_instances->update_subone(1, transparency_offset, sizeof(float), &blink);

    {
      int font_size = round((sin(time * 2) + 1) * 14 + 8);
      char buff[64];
      snprintf(buff, sizeof(buff), "The The quick brown %d foxes, jump over the lazy dog. fi", font_size);
      if (roboto && s.bake(b, hello_world, roboto, font_size, buff))
        w.invalidate(true);
    }

    static int frameCount = 0;
    static auto lastReport = display::clock::now();

    frameCount++;
    auto diffReport = currentTime - lastReport;
    if (roboto) {
      double fps = double(frameCount) / std::chrono::duration<double>(diffReport).count();
      char buff[32];
      snprintf(buff, sizeof(buff), "fps: %f", fps);
      if (s.bake(b, fps_counter, roboto, 12, buff))
        w.invalidate(true);
      if (diffReport > 1s) {
        lastReport = currentTime;
        frameCount = 0;
      }
    }

    return false;
  });

  w.on_resize.connect([&](const uvec2 &_size) {
    //std::cout << "resize " << to_string(_size) << std::endl;
    proj_ubo new_ubo;
    new_ubo.proj_ = ortho<float>(0, _size.x, 0, _size.y);

    ubo->set(new_ubo);
    return false;
  });

  w.on_expose.connect([](const uvec4 &) {
    //std::cout << "expose " << to_string(_bounds) << std::endl;
    return false;
  });

  w.on_key.connect([&](keycode, keysym _key, bool _press) {
    std::cout << "w keysym " << _press << '\t' << keysym_name(_key) << std::endl;
    if (_key == KSYM_ESC && !_press)
      w.close();
    else if (_key == KSYM_UP && !_press)
      w.pause();
    else if (_key == KSYM_LEFT && !_press)
      w.maximize(false);
    else if (_key == KSYM_RIGHT && !_press)
      w.maximize(true);
    return true;
  });

  w2.on_key.connect([&w2, &d](keycode, keysym _key, bool _press) {
    std::cout << "w2 keysym " << _press << '\t' << keysym_name(_key) << std::endl;
    if (_key == KSYM_ESC && !_press)
      w2.close();
    else if (_key == KSYM_UP && !_press)
      w2.pause();
    else if (_key == KSYM_LEFT && !_press)
      w2.maximize(false);
    else if (_key == KSYM_RIGHT && !_press)
      w2.maximize(true);
    else if (_key == KSYM_F11 && !_press)
      w2.fullscreen(true);
    else if (_key == KSYM_F12 && !_press)
      w2.fullscreen(false);
    return true;
  });

  w.on_char.connect([](char32_t c) {
    std::cout << "char '" << (char*)to_utf8(c).data() << "' (0x" << std::hex << uint32_t(c) << std::dec << ')' << std::endl;
    return true;
  });

  w.on_mouse.connect([](uint8_t _button, mouse_event_type _type, vec2 _pos) {
    if (_type != MMOVE)
      std::cout << "mouse " << std::to_string(_button) << ", type: " << _type << ", pos: " << to_string(_pos) << std::endl;
    return true;
  });

  w.on_close.connect([] {
    std::cout << "closing window..." << std::endl;
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

  w.on_mouse.connect([&w](uint8_t _button, mouse_event_type _type, vec2 _pos) {
    if (_type == MUP) {
      static int current_cursor = CDEFAULT;
      if (++current_cursor > CZOOM_OUT)
        current_cursor = CDEFAULT;
      w.cursor(static_cast<cursor_type>(current_cursor));
      std::cout << "current cursor " << cursor_css_name(static_cast<cursor_type>(current_cursor)) << std::endl;
    }
    return true;
  });

  w.on_key.connect([&w](keycode, keysym _key, bool _pressed) {
    if (_key == KSYM_C && _pressed) {
      std::cout << "Offering to clipboard" << std::endl;

      static const char text_html[] = R"(<meta http-equiv="content-type" content="text/html; charset=UTF-8"><html><body><b>Hello</b> from <i>libhut</i></body></html>)";
      static const char text_plain[] = "Hello from libhut";
      w.clipboard_offer(clipboard_formats{} | FTEXT_HTML | FTEXT_PLAIN, [](clipboard_format _mime, window::clipboard_sender &_sender) {
        std::cout << "Writing to clipboard in " << format_mime_type(_mime) << std::endl;
        if (_mime == FTEXT_HTML) {
          _sender.write({(uint8_t*)text_html, sizeof(text_html)});
        }
        else if (_mime == FTEXT_PLAIN) {
          _sender.write({(uint8_t*)text_plain, sizeof(text_plain)});
        }
      });
    }
    if (_key == KSYM_V && _pressed) {
      std::cout << "Receiving from clipboard" << std::endl;

      w.clipboard_receive(clipboard_formats{} | FTEXT_HTML | FTEXT_PLAIN, [](clipboard_format _mime, window::clipboard_receiver &_receiver) {
        std::cout << "Clipboard contents in " << format_mime_type(_mime) << ": ";
        uint8_t buffer[2048];
        size_t read_bytes;
        while ((read_bytes = _receiver.read(buffer))) {
          std::cout << std::string_view{(const char*)buffer, read_bytes};
        }
        std::cout << std::endl;
      });
    }

    if (_key == KSYM_P && _pressed) {
      profiling::threads_data::request_dump();
    }

    return true;
  });

  w2.on_mouse.connect([&w2](uint8_t _button, mouse_event_type _type, vec2 _coords) {
    edge coords_edge;
    if (_coords.x < 50)
      coords_edge |= LEFT;
    if (_coords.y < 50)
      coords_edge |= TOP;
    if (w2.size().x - _coords.x < 50)
      coords_edge |= RIGHT;
    if (w2.size().y - _coords.y < 50)
      coords_edge |= BOTTOM;
    w2.cursor(edge_cursor(coords_edge));
    if (!coords_edge)
      w2.interactive_move();
    else
      w2.interactive_resize(coords_edge);
    return true;
  });

  auto result = d.dispatch();
  load_res.join();
  std::cout << "done!" << std::endl;
  return result;
}
