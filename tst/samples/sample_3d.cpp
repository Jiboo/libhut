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

#include <numbers>

#include "hut/utils/color.hpp"

#include "hut/display.hpp"
#include "hut/pipeline.hpp"
#include "hut/window.hpp"

#include "tst_pipelines.hpp"

using namespace hut;

int main(int /*unused*/, char ** /*unused*/) {
  display d("hut demo3d");
  auto    b = std::make_shared<buffer>(d, 1024 * 1024);

  window_params params;
  params.flags_.set(window_params::FDEPTH);
  params.flags_.set(window_params::FMULTISAMPLING);
  params.flags_.set(window_params::FTRANSPARENT);

  window w(d, params);
  w.clear_color({0.1f, 0.1f, 0.1f, 0.0f});
  w.title(u8"hut demo3d win");

  auto indices = b->allocate<u16>(36);
  {
    auto iupdator = indices->update();
    iupdator.set({
        0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 7, 6, 5, 5, 4, 7, 4, 0, 3, 3, 7, 4, 4, 5, 1, 1, 0, 4, 3, 2, 6, 6, 7, 3,
    });
  }

  auto ubo = b->allocate<vp_ubo>(1, d.ubo_align());
  ubo->set(vp_ubo{
      perspective(radians(45.0f), float(w.size().x) / float(w.size().y), 0.001f, 1000.0f),
      lookAt(vec3{0, 0, 5}, vec3{0, 0, 0}, vec3{0, -1, 0}),
  });

  auto rgb3d_pipeline  = std::make_unique<pipeline_rgb3d>(w);
  auto rgb3d_instances = b->allocate<pipeline_rgb3d::instance>(1);
  auto rgb3d_vertices  = b->allocate<pipeline_rgb3d::vertex>(8);
  rgb3d_vertices->set({
      // back
      pipeline_rgb3d::vertex{{-1, -1, 1}, {0, 0, 0}},
      pipeline_rgb3d::vertex{{1, -1, 1}, {1, 0, 0}},
      pipeline_rgb3d::vertex{{1, 1, 1}, {0, 1, 0}},
      pipeline_rgb3d::vertex{{-1, 1, 1}, {1, 1, 0}},
      // front
      pipeline_rgb3d::vertex{{-1, -1, -1}, {0, 0, 1}},
      pipeline_rgb3d::vertex{{1, -1, -1}, {1, 0, 1}},
      pipeline_rgb3d::vertex{{1, 1, -1}, {0, 1, 1}},
      pipeline_rgb3d::vertex{{-1, 1, -1}, {1, 1, 1}},
  });
  rgb3d_instances->set({
      pipeline_rgb3d::instance{identity<mat4>()},
  });
  rgb3d_pipeline->write(0, ubo);

  image_params skybox_params;
  skybox_params.flags_             = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  skybox_params.layers_            = 6;
  skybox_params.size_              = {4, 4};
  skybox_params.format_            = VK_FORMAT_R8G8B8A8_UNORM;
  auto                skybox_image = std::make_shared<image>(d, skybox_params);
  u8vec4_rgba         pixels[16];
  std::span<const u8> pixels_ref{&pixels[0][0], 16 * sizeof(u8vec4_rgba)};
  std::fill(std::begin(pixels), std::end(pixels), u8vec4_rgba{0xFF, 0x00, 0x00, 0x80});
  skybox_image->update({{0, 0, 4, 4}, 0, 0}, pixels_ref, 4 * sizeof(u8vec4_rgba));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4_rgba{0x00, 0xFF, 0x00, 0x80});
  skybox_image->update({{0, 0, 4, 4}, 0, 1}, pixels_ref, 4 * sizeof(u8vec4_rgba));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4_rgba{0xFF, 0xFF, 0x00, 0x80});
  skybox_image->update({{0, 0, 4, 4}, 0, 2}, pixels_ref, 4 * sizeof(u8vec4_rgba));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4_rgba{0x00, 0x00, 0xFF, 0x80});
  skybox_image->update({{0, 0, 4, 4}, 0, 3}, pixels_ref, 4 * sizeof(u8vec4_rgba));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4_rgba{0xFF, 0x00, 0xFF, 0x80});
  skybox_image->update({{0, 0, 4, 4}, 0, 4}, pixels_ref, 4 * sizeof(u8vec4_rgba));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4_rgba{0x00, 0xFF, 0xFF, 0x80});
  skybox_image->update({{0, 0, 4, 4}, 0, 5}, pixels_ref, 4 * sizeof(u8vec4_rgba));

  auto skybox_sampler  = std::make_shared<sampler>(d);
  auto skybox_pipeline = std::make_unique<pipeline_skybox>(w);
  auto skybox_vertices = b->allocate<pipeline_skybox::vertex>(8);
  skybox_vertices->set({
      // back
      pipeline_skybox::vertex{{-9, -9, 9}, {-1, -1, 1}},
      pipeline_skybox::vertex{{9, -9, 9}, {1, -1, 1}},
      pipeline_skybox::vertex{{9, 9, 9}, {1, 1, 1}},
      pipeline_skybox::vertex{{-9, 9, 9}, {-1, 1, 1}},
      // front
      pipeline_skybox::vertex{{-9, -9, -9}, {-1, -1, -1}},
      pipeline_skybox::vertex{{9, -9, -9}, {1, -1, -1}},
      pipeline_skybox::vertex{{9, 9, -9}, {1, 1, -1}},
      pipeline_skybox::vertex{{-9, 9, -9}, {-1, 1, -1}},
  });
  skybox_pipeline->write(0, ubo, skybox_image, skybox_sampler);

  w.on_draw_.connect([&](VkCommandBuffer _buffer) {
    rgb3d_pipeline->draw(_buffer, 0, indices, rgb3d_instances, rgb3d_vertices);
    skybox_pipeline->draw(_buffer, 0, indices, pipeline_skybox::shared_instances{}, skybox_vertices);
    return false;
  });

  w.on_resize_.connect([&](const u16vec2_px &_size, u32 _scale) {
    mat4 new_proj = perspective(radians(45.0f), (float)_size.x / (float)_size.y, 0.001f, 1000.0f);
    ubo->set_subone(0, offsetof(vp_ubo, proj_), sizeof(mat4), &new_proj);
    return false;
  });

  w.on_key_.connect([&](keycode, keysym _c, bool _press) {
    if (_c == KSYM_ESC && !_press)
      w.close();
    return true;
  });

  vec2 camera_rot      = {0, 0};
  vec2 down_pos        = {0, 0};
  bool button_clicked  = false;
  bool dragging_window = false;
  w.on_mouse_.connect([&](u8 _button, mouse_event_type _type, vec2 _pos) {
    if (_type == MUP) {
      button_clicked = false;
      return true;
    }
    if (_type == MDOWN) {
      button_clicked  = true;
      down_pos        = _pos;
      dragging_window = _pos.x < 20;
      return true;
    } else if (_type == MMOVE && button_clicked) {
      if (dragging_window) {
        w.interactive_move();
      } else {
        vec2 offset{_pos - down_pos};
        camera_rot += offset / vec2(w.size()) * float(std::numbers::pi);
        constexpr float MAX_Y = std::numbers::pi / 2 - 0.01;
        camera_rot.y          = clamp(camera_rot.y, -MAX_Y, MAX_Y);
        down_pos              = _pos;
        vec3 orbit_base       = vec3(0, 0, 5);
        mat4 mrot             = mat4(1);
        mrot                  = rotate(mrot, -camera_rot.x, vec3{0, 1, 0});
        mrot                  = rotate(mrot, -camera_rot.y, vec3{1, 0, 0});
        vec3 orbit            = mrot * vec4(orbit_base, 1);
        mat4 m                = lookAt(orbit, vec3(0, 0, 0), vec3{0, -1, 0});
        ubo->set_subone(0, offsetof(vp_ubo, view_), sizeof(mat4), &m);
        w.invalidate(false);
      }
      return true;
    }

    return false;
  });

  return d.dispatch();
}
