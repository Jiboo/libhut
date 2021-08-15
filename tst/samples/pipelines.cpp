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
#include "hut/shaping.hpp"
#include "hut/window.hpp"

#include "hut_imgdec.hpp"

#include "tst_woff2.hpp"
#include "tst_png.hpp"
#include "tst_events.hpp"

using namespace hut;

int main(int, char **) {
  std::cout << std::fixed << std::setprecision(1);

  display d("hut demo");
  window_params wp;
  wp.flags_.set(window_params::FTRANSPARENT);
  window w(d, wp);
  w.clear_color({0.1f, 0.1f, 0.1f, 0});
  w.title("hut demo win1");

  shared_sampler samp = std::make_shared<sampler>(d);

  auto b = d.alloc_buffer(1024*1024);

  auto indices = b->allocate<uint16_t>(6);
  indices->set({0, 1, 2, 2, 1, 3});

  proj_ubo default_ubo;
  default_ubo.proj_ = ortho<float>(0.f, float(w.size().x), 0.f, float(w.size().y));
  shared_ref<proj_ubo> ubo = d.alloc_ubo(b, default_ubo);

  install_test_events(d, w, ubo);

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
    rgb::instance{make_transform_mat4({  0, 100}, {1,1,1})},
    rgb::instance{make_transform_mat4({100,   0}, {1,1,1})}
  });
  rgb_pipeline->write(0, ubo);

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
    rgba::instance{make_transform_mat4({  0,   0}, {1,1,1})},
    rgba::instance{make_transform_mat4({100, 100}, {1,1,1})}
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
    tex_rgb::instance{make_transform_mat4({  0, 200}, {100,100,1})},
    tex_rgb::instance{make_transform_mat4({100, 300}, {100,100,1})},
  });
  tex2_rgb_instances->set({
    tex_rgb::instance{make_transform_mat4({100, 200}, {100,100,1})},
    tex_rgb::instance{make_transform_mat4({  0, 300}, {100,100,1})},
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
    tex_rgba::instance{make_transform_mat4({  0, 400}, {100,100,1})},
    tex_rgba::instance{make_transform_mat4({100, 500}, {100,100,1})},
  });
  tex2_rgba_instances->set({
    tex_rgba::instance{make_transform_mat4({100, 400}, {100,100,1})},
    tex_rgba::instance{make_transform_mat4({  0, 500}, {100,100,1})},
  });

  shared_atlas r8_atlas = std::make_shared<atlas_pool>(d, image_params{.format_ = VK_FORMAT_R8_UNORM});
  auto tmask_pipeline = std::make_unique<tex_mask>(w);
  tmask_pipeline->write(0, ubo, r8_atlas->image(0), samp);

  auto text_instances = b->allocate<tex_mask::instance>(2);
  auto icons_instances = b->allocate<tex_mask::instance>(2);
  auto fps_instances = b->allocate<tex_mask::instance>(2);
  auto atlas_instances = b->allocate<tex_mask::instance>(1);
  auto atlas_vertices = b->allocate<tex_mask::vertex>(4);
  text_instances->set({
    tex_mask::instance{make_transform_mat4({  1, 101}, {1,1,1}), {0, 0, 0, 1}},
    tex_mask::instance{make_transform_mat4({  0, 100}, {1,1,1}), {1, 1, 1, 1}},
  });
  icons_instances->set({
    tex_mask::instance{make_transform_mat4({  1, 201}, {1,1,1}), {0, 0, 0, 1}},
    tex_mask::instance{make_transform_mat4({  0, 200}, {1,1,1}), {1, 1, 1, 1}},
  });
  fps_instances->set({
    tex_mask::instance{make_transform_mat4({  6, 17}, {1,1,1}), {0, 0, 0, 1}},
    tex_mask::instance{make_transform_mat4({  5, 16}, {1,1,1}), {1, 1, 1, 1}},
  });
  atlas_vertices->set({
    tex_mask::vertex{{0, 0}, {0, 0}},
    tex_mask::vertex{{0, 1}, {0, 1}},
    tex_mask::vertex{{1, 0}, {1, 0}},
    tex_mask::vertex{{1, 1}, {1, 1}},
  });
  atlas_instances->set(tex_mask::instance{make_transform_mat4({200, 0}, {r8_atlas->image(0)->size(), 1}), {1, 0, 0, 1}});

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
    box_rgba::instance{make_transform_mat4({300, 400}, {1,1,1}), {8, 8}, {300, 400, 400, 500}},
  });
  shadow_instances->set({
    box_rgba::instance{make_transform_mat4({300, 400}, {1,1,1}), {8, 8}, {300, 400, 400, 500}},
  });
  box_rgba_pipeline->write(0, ubo);

  shared_image tex1, tex2;
  shared_font roboto, material_icons;

  using myshaper = shaper<uint16_t, tex_mask::vertex>;
  myshaper s;
  // in this context to keep a ref on buffers
  myshaper::result hello_world;
  myshaper::result fps_counter;
  myshaper::result icons_test;

  std::atomic_bool tex1_ready = false, tex2_ready = false;

  std::thread load_res([&]() {
    tex1 = imgdec::load_png(d, tst_png::tex1_png);
    tex_pipeline->write(0, ubo, tex1, samp);
    tex_rgb_pipeline->write(0, ubo, tex1, samp);
    tex_rgba_pipeline->write(0, ubo, tex1, samp);
    tex1_instances->set({
      tex::instance{make_transform_mat4({200,   0}, {tex1->size() / 2, 1})},
      tex::instance{make_transform_mat4({300, 100}, {tex1->size() / 2, 1})},
    });
    tex1_ready = true;
    w.invalidate(true);  // will force to call on_draw on the next frame

    {
      auto update = tex1->prepare_update({{100, 100, 200, 200}});
      for (int y = 0; y < 100; y++) {
        auto *row = (u8vec4 *) (update.data() + y * update.staging_row_pitch());
        for (int x = 0; x < 100; x++)
          *(row + x) = u8vec4{0x80, 0, 0, 0x80};
      }
      w.invalidate(true);  // will force to call on_draw on the next frame
    }

    tex2 = imgdec::load_png(d, tst_png::tex2_png);
    tex_pipeline->write(1, ubo, tex2, samp);
    tex_rgb_pipeline->write(1, ubo, tex2, samp);
    tex_rgba_pipeline->write(1, ubo, tex2, samp);
    tex2_instances->set({
      tex::instance{make_transform_mat4({300,   0}, {tex2->size() / 2, 1})},
      tex::instance{make_transform_mat4({400, 100}, {tex2->size() / 2, 1})},
    });
    tex2_ready = true;
    w.invalidate(true);  // will force to call on_draw on the next frame

    roboto = std::make_shared<font>(d, tst_woff2::Roboto_Regular_woff2.data(), tst_woff2::Roboto_Regular_woff2.size(), r8_atlas, true);
    material_icons = std::make_shared<font>(d, tst_woff2::materialdesignicons_webfont_woff2.data(), tst_woff2::materialdesignicons_webfont_woff2.size(), r8_atlas, false);
    s.bake(myshaper::context{b, icons_test, material_icons, 16, "\uE834\uE835\uE836\uE837"});
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

  w.on_frame.connect([&](display::duration) {
    d.post([&w](auto){
      w.invalidate(false);  // asks for another frame without a redraw to continue animations and fps count
    });

    static auto startTime = display::clock::now();

    auto currentTime = display::clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    float angle = radians(time * 10);
    if (tex1_ready) {
      tex1_instances->update_one(1, tex::instance{make_transform_mat4({300, 100}, angle, tex1->size()/4, {tex1->size()/2, 1})});
    }
    if (tex2_ready) {
      tex2_instances->update_one(1, tex::instance{make_transform_mat4({400, 100}, angle, tex2->size()/4, {tex2->size()/2, 1})});
    }

    float border_radius = (sin(time * 4) + 1) * 20 + 1;
    vec2 new_box_params = {border_radius, 1};
    box_rgba_instances->update_subone(0, offsetof(box_rgba::instance, params_), sizeof(new_box_params), &new_box_params);

    new_box_params[1] = (sin(time * 4) + 1) * 10 + 1;
    shadow_instances->update_subone(0, offsetof(box_rgba::instance, params_), sizeof(new_box_params), &new_box_params);

    float blink = (sin(time * 4) + 1) / 2;
    constexpr uint transparency_offset = offsetof(tex_mask::instance, col_) + 3 * sizeof(float);
    icons_instances->update_subone(1, transparency_offset, sizeof(float), &blink);

    {
      int font_size = (int)round((sin(time * 2) + 1) * 14 + 8);
      char buff[64];
      snprintf(buff, sizeof(buff), "The The quick brown %d foxes, jump over the lazy dog. fi", font_size);
      if (roboto && s.bake(myshaper::context{b, hello_world, roboto, (u8)font_size, buff}))
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
      if (s.bake(myshaper::context{b, fps_counter, roboto, 12, buff}))
        w.invalidate(true);
      if (diffReport > 1s) {
        lastReport = currentTime;
        frameCount = 0;
      }
    }

    return false;
  });

  auto result = d.dispatch();
  load_res.join();
  return result;
}
