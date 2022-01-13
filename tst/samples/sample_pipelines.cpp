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
#include "hut/window.hpp"

#include "hut/imgdec/imgdec.hpp"

#include "tst_events.hpp"
#include "tst_pipelines.hpp"
#include "tst_png.hpp"

using namespace hut;

int main(int, char **) {
  std::cout << std::fixed << std::setprecision(1);

  display d("hut demo");
  auto    b = std::make_shared<buffer>(d, 1024 * 1024);

  window_params wp;
  wp.flags_.set(window_params::FTRANSPARENT);
  window w(d, wp);
  w.clear_color({0.1f, 0.1f, 0.1f, 0});
  w.title(u8"hut demo win1");

  shared_sampler samp = std::make_shared<sampler>(d);

  auto indices = b->allocate<u16>(6);
  indices->set({0, 1, 2, 2, 1, 3});

  auto ubo = b->allocate<proj_ubo>(1, d.ubo_align());
  ubo->set(proj_ubo{w.size()});

  install_test_events(d, w, ubo);

  auto rgb_pipeline  = std::make_unique<pipeleine_rgb>(w);
  auto rgb_instances = b->allocate<pipeleine_rgb::instance>(2);
  auto rgb_vertices  = b->allocate<pipeleine_rgb::vertex>(4);
  rgb_vertices->set({
      pipeleine_rgb::vertex{{0, 0}, {1, 0, 0}},
      pipeleine_rgb::vertex{{0, 100}, {0, 0, 1}},
      pipeleine_rgb::vertex{{100, 0}, {0, 1, 0}},
      pipeleine_rgb::vertex{{100, 100}, {1, 1, 1}},
  });
  rgb_instances->set({pipeleine_rgb::instance{make_transform_mat4({0, 100}, {1, 1, 1})},
                      pipeleine_rgb::instance{make_transform_mat4({100, 0}, {1, 1, 1})}});
  rgb_pipeline->write(0, ubo);

  auto rgba_pipeline  = std::make_unique<pipeleine_rgba>(w);
  auto rgba_instances = b->allocate<pipeleine_rgba::instance>(2);
  auto rgba_vertices  = b->allocate<pipeleine_rgba::vertex>(4);
  rgba_vertices->set({
      pipeleine_rgba::vertex{{0, 0}, {1, 0, 0, 0.25}},
      pipeleine_rgba::vertex{{0, 100}, {0, 0, 1, 0.25}},
      pipeleine_rgba::vertex{{100, 0}, {0, 1, 0, 0.25}},
      pipeleine_rgba::vertex{{100, 100}, {1, 1, 1, 0.25}},
  });
  rgba_instances->set({pipeleine_rgba::instance{make_transform_mat4({0, 0}, {1, 1, 1})},
                       pipeleine_rgba::instance{make_transform_mat4({100, 100}, {1, 1, 1})}});
  rgba_pipeline->write(0, ubo);

  auto tex_pipeline = std::make_unique<pipeleine_tex>(w);
  tex_pipeline->resize_descriptors(2);
  auto tex1_instances = b->allocate<pipeleine_tex::instance>(2);
  auto tex2_instances = b->allocate<pipeleine_tex::instance>(2);
  auto tex_vertices   = b->allocate<pipeleine_tex::vertex>(4);
  tex_vertices->set({
      pipeleine_tex::vertex{{0, 0}, {0, 0}},
      pipeleine_tex::vertex{{0, 1}, {0, 1}},
      pipeleine_tex::vertex{{1, 0}, {1, 0}},
      pipeleine_tex::vertex{{1, 1}, {1, 1}},
  });

  auto tex_rgb_pipeline = std::make_unique<pipeleine_tex_rgb>(w);
  tex_rgb_pipeline->resize_descriptors(2);
  auto tex1_rgb_instances = b->allocate<pipeleine_tex_rgb::instance>(2);
  auto tex2_rgb_instances = b->allocate<pipeleine_tex_rgb::instance>(2);
  auto tex_rgb_vertices   = b->allocate<pipeleine_tex_rgb::vertex>(4);
  tex_rgb_vertices->set({
      pipeleine_tex_rgb::vertex{{0, 0}, {0, 0}, {1, 0, 0}},
      pipeleine_tex_rgb::vertex{{0, 1}, {0, 1}, {0, 0, 1}},
      pipeleine_tex_rgb::vertex{{1, 0}, {1, 0}, {0, 1, 0}},
      pipeleine_tex_rgb::vertex{{1, 1}, {1, 1}, {1, 1, 1}},
  });
  tex1_rgb_instances->set({
      pipeleine_tex_rgb::instance{make_transform_mat4({0, 200}, {100, 100, 1})},
      pipeleine_tex_rgb::instance{make_transform_mat4({100, 300}, {100, 100, 1})},
  });
  tex2_rgb_instances->set({
      pipeleine_tex_rgb::instance{make_transform_mat4({100, 200}, {100, 100, 1})},
      pipeleine_tex_rgb::instance{make_transform_mat4({0, 300}, {100, 100, 1})},
  });

  auto tex_rgba_pipeline = std::make_unique<pipeleine_tex_rgba>(w);
  tex_rgba_pipeline->resize_descriptors(2);
  auto tex1_rgba_instances = b->allocate<pipeleine_tex_rgba::instance>(2);
  auto tex2_rgba_instances = b->allocate<pipeleine_tex_rgba::instance>(2);
  auto tex_rgba_vertices   = b->allocate<pipeleine_tex_rgba::vertex>(4);
  tex_rgba_vertices->set({
      pipeleine_tex_rgba::vertex{{0, 0}, {0, 0}, {1, 0, 0, 0.25}},
      pipeleine_tex_rgba::vertex{{0, 1}, {0, 1}, {0, 0, 1, 0.25}},
      pipeleine_tex_rgba::vertex{{1, 0}, {1, 0}, {0, 1, 0, 0.25}},
      pipeleine_tex_rgba::vertex{{1, 1}, {1, 1}, {1, 1, 1, 0.25}},
  });
  tex1_rgba_instances->set({
      pipeleine_tex_rgba::instance{make_transform_mat4({0, 400}, {100, 100, 1})},
      pipeleine_tex_rgba::instance{make_transform_mat4({100, 500}, {100, 100, 1})},
  });
  tex2_rgba_instances->set({
      pipeleine_tex_rgba::instance{make_transform_mat4({100, 400}, {100, 100, 1})},
      pipeleine_tex_rgba::instance{make_transform_mat4({0, 500}, {100, 100, 1})},
  });

  auto box_rgba_pipeline = std::make_unique<pipeleine_box_rgba>(w);
  auto box_rgba_vertices = b->allocate<pipeleine_box_rgba::vertex>(4);
  auto shadow_vertices   = b->allocate<pipeleine_box_rgba::vertex>(4);
  box_rgba_vertices->set({
      pipeleine_box_rgba::vertex{{0, 0}, {1, 0, 0, 1}},
      pipeleine_box_rgba::vertex{{0, 100}, {0, 0, 1, 1}},
      pipeleine_box_rgba::vertex{{100, 0}, {0, 1, 0, 1}},
      pipeleine_box_rgba::vertex{{100, 100}, {1, 1, 1, 1}},
  });
  shadow_vertices->set({
      pipeleine_box_rgba::vertex{{0, 0}, {0, 0, 0, 1}},
      pipeleine_box_rgba::vertex{{0, 100}, {0, 0, 0, 1}},
      pipeleine_box_rgba::vertex{{100, 0}, {0, 0, 0, 1}},
      pipeleine_box_rgba::vertex{{100, 100}, {0, 0, 0, 1}},
  });
  auto box_rgba_instances = b->allocate<pipeleine_box_rgba::instance>(1);
  auto shadow_instances   = b->allocate<pipeleine_box_rgba::instance>(1);
  box_rgba_instances->set({
      pipeleine_box_rgba::instance{make_transform_mat4({300, 400}, {1, 1, 1}), {8, 8}, {300, 400, 400, 500}},
  });
  shadow_instances->set({
      pipeleine_box_rgba::instance{make_transform_mat4({300, 400}, {1, 1, 1}), {8, 8}, {300, 400, 400, 500}},
  });
  box_rgba_pipeline->write(0, ubo);

  shared_image     tex1, tex2;
  std::atomic_bool tex1_ready = false, tex2_ready = false;

  std::thread load_res([&]() {
    tex1 = imgdec::load_png(d, tst_png::tex1_png);
    tex_pipeline->write(0, ubo, tex1, samp);
    tex_rgb_pipeline->write(0, ubo, tex1, samp);
    tex_rgba_pipeline->write(0, ubo, tex1, samp);
    tex1_instances->set({
        pipeleine_tex::instance{make_transform_mat4({200, 0}, {tex1->size() / 2, 1})},
        pipeleine_tex::instance{make_transform_mat4({300, 100}, {tex1->size() / 2, 1})},
    });
    tex1_ready = true;
    w.invalidate(true);  // will force to call on_draw on the next frame

    {
      auto update = tex1->update({{100, 100, 200, 200}});
      for (int y = 0; y < 100; y++) {
        auto *row = (u8vec4 *)(update.data() + y * update.staging_row_pitch());
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
        pipeleine_tex::instance{make_transform_mat4({300, 0}, {tex2->size() / 2, 1})},
        pipeleine_tex::instance{make_transform_mat4({400, 100}, {tex2->size() / 2, 1})},
    });
    tex2_ready = true;
    w.invalidate(true);  // will force to call on_draw on the next frame
  });

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
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

    return false;
  });

  w.on_frame.connect([&](display::duration) {
    d.post([&w](auto) {
      w.invalidate(false);  // asks for another frame without a redraw to continue animations and fps count
    });

    static auto startTime = display::clock::now();

    auto  currentTime = display::clock::now();
    float time        = std::chrono::duration<float>(currentTime - startTime).count();

    float angle = radians(time * 10);
    if (tex1_ready) {
      tex1_instances->set_one(
          1, pipeleine_tex::instance{make_transform_mat4({300, 100}, angle, tex1->size() / 4, {tex1->size() / 2, 1})});
    }
    if (tex2_ready) {
      tex2_instances->set_one(
          1, pipeleine_tex::instance{make_transform_mat4({400, 100}, angle, tex2->size() / 4, {tex2->size() / 2, 1})});
    }

    float border_radius  = (sin(time * 4) + 1) * 20 + 1;
    vec2  new_box_params = {border_radius, 1};
    box_rgba_instances->set_subone(0, offsetof(pipeleine_box_rgba::instance, params_), sizeof(new_box_params),
                                   &new_box_params);

    new_box_params[1] = (sin(time * 4) + 1) * 10 + 1;
    shadow_instances->set_subone(0, offsetof(pipeleine_box_rgba::instance, params_), sizeof(new_box_params),
                                 &new_box_params);

    return false;
  });

  auto result = d.dispatch();
  load_res.join();
  return result;
}
