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
#include <numbers>

#include "hut/display.hpp"
#include "hut/pipeline.hpp"
#include "hut/font.hpp"
#include "hut/window.hpp"

#include "demo_shaders_refl.hpp"

using namespace hut;

struct vp_ubo {
  mat4 proj_;
  mat4 view_;
};

using rgb3d = hut::pipeline<vp_ubo, uint16_t, hut::demo_shaders::rgb3d_vert_spv_refl, hut::hut_shaders::rgb_frag_spv_refl>;
using skybox = hut::pipeline<vp_ubo, uint16_t, hut::demo_shaders::skybox_vert_spv_refl, hut::demo_shaders::skybox_frag_spv_refl, const shared_image&, const shared_sampler&>;

int main(int, char **) {
  display d("hut demo3d");

  window_params params;
  params.flags_.set(window_params::FDEPTH);
  params.flags_.set(window_params::FMULTISAMPLING);
  params.flags_.set(window_params::FTRANSPARENT);

  window w(d, params);
  w.clear_color({0.1f, 0.1f, 0.1f, 0.0f});
  w.title("hut demo3d win");

  auto b = d.alloc_buffer(1024*1024);

  auto indices = b->allocate<uint16_t>(36);
  indices->set({
    0, 1, 2, 2, 3, 0,
    1, 5, 6, 6, 2, 1,
    7, 6, 5, 5, 4, 7,
    4, 0, 3, 3, 7, 4,
    4, 5, 1, 1, 0, 4,
    3, 2, 6, 6, 7, 3,
  });

  vp_ubo default_ubo{
    perspective(glm::radians(45.0f), w.size().x / (float) w.size().y, 0.001f, 1000.0f),
    lookAt(vec3{0, 0, 5}, vec3{0, 0, 0}, vec3{0, -1, 0}),
  };
  shared_ref<vp_ubo> ubo = d.alloc_ubo(b, default_ubo);

  auto rgb3d_pipeline = std::make_unique<rgb3d>(w);
  auto rgb3d_instances = b->allocate<rgb3d::instance>(1);
  auto rgb3d_vertices = b->allocate<rgb3d::vertex>(8);
  rgb3d_vertices->set({
    // back
    rgb3d::vertex{{-1, -1, 1}, {0, 0, 0}},
    rgb3d::vertex{{ 1, -1, 1}, {1, 0, 0}},
    rgb3d::vertex{{ 1,  1, 1}, {0, 1, 0}},
    rgb3d::vertex{{-1,  1, 1}, {1, 1, 0}},
    // front
    rgb3d::vertex{{-1, -1, -1}, {0, 0, 1}},
    rgb3d::vertex{{ 1, -1, -1}, {1, 0, 1}},
    rgb3d::vertex{{ 1,  1, -1}, {0, 1, 1}},
    rgb3d::vertex{{-1,  1, -1}, {1, 1, 1}},
  });
  rgb3d_instances->set({
    rgb3d::instance{glm::identity<mat4>()},
  });
  rgb3d_pipeline->write(0, ubo);

  image_params skybox_params;
  skybox_params.flags_ = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  skybox_params.layers_ = 6;
  skybox_params.size_ = {4, 4};
  skybox_params.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  auto skybox_image = std::make_shared<image>(d, skybox_params);
  u8vec4 pixels[16];
  std::fill(std::begin(pixels), std::end(pixels), u8vec4{0xFF, 0x00, 0x00, 0xFF});
  skybox_image->update({{0, 0, 4, 4}, 0, 0}, (uint8_t*)pixels, 4 * sizeof(u8vec4));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4{0x00, 0xFF, 0x00, 0xFF});
  skybox_image->update({{0, 0, 4, 4}, 0, 1}, (uint8_t*)pixels, 4 * sizeof(u8vec4));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4{0xFF, 0xFF, 0x00, 0xFF});
  skybox_image->update({{0, 0, 4, 4}, 0, 2}, (uint8_t*)pixels, 4 * sizeof(u8vec4));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4{0x00, 0x00, 0xFF, 0xFF});
  skybox_image->update({{0, 0, 4, 4}, 0, 3}, (uint8_t*)pixels, 4 * sizeof(u8vec4));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4{0xFF, 0x00, 0xFF, 0xFF});
  skybox_image->update({{0, 0, 4, 4}, 0, 4}, (uint8_t*)pixels, 4 * sizeof(u8vec4));
  std::fill(std::begin(pixels), std::end(pixels), u8vec4{0x00, 0xFF, 0xFF, 0xFF});
  skybox_image->update({{0, 0, 4, 4}, 0, 5}, (uint8_t*)pixels, 4 * sizeof(u8vec4));

  auto skybox_sampler = std::make_shared<sampler>(d);
  auto skybox_pipeline = std::make_unique<skybox>(w);
  auto skybox_vertices = b->allocate<skybox::vertex>(8);
  skybox_vertices->set({
    // back
    skybox::vertex{{-9, -9, 9}, {-1, -1,  1}},
    skybox::vertex{{ 9, -9, 9}, { 1, -1,  1}},
    skybox::vertex{{ 9,  9, 9}, { 1,  1,  1}},
    skybox::vertex{{-9,  9, 9}, {-1,  1,  1}},
    // front
    skybox::vertex{{-9, -9, -9}, {-1, -1, -1}},
    skybox::vertex{{ 9, -9, -9}, { 1, -1, -1}},
    skybox::vertex{{ 9,  9, -9}, { 1,  1, -1}},
    skybox::vertex{{-9,  9, -9}, {-1,  1, -1}},
  });
  skybox_pipeline->write(0, ubo, skybox_image, skybox_sampler);

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
    rgb3d_pipeline->draw(_buffer, 0, indices, rgb3d_instances, rgb3d_vertices);
    skybox_pipeline->draw(_buffer, 0, indices, skybox::shared_instances{}, skybox_vertices);
    return false;
  });

  w.on_resize.connect([&](const uvec2 &_size) {
    mat4 new_proj = glm::perspective(glm::radians(45.0f), _size.x / (float) _size.y, 0.1f, 10.0f);
    ubo->update_subone(0, offsetof(vp_ubo, proj_), sizeof(mat4), &new_proj);
    return false;
  });

  w.on_key.connect([&](keycode, keysym c, bool _press) {
    if (c == KSYM_ESC && !_press)
      w.close();
    return true;
  });

  vec2 camera_rot = {0, 0};
  vec2 down_pos = {0, 0};
  bool button_clicked = false;
  w.on_mouse.connect([&](uint8_t _button, mouse_event_type _type, vec2 _pos) {
    if (_type == MUP) {
      button_clicked = false;
      return true;
    }
    else if (_type == MDOWN) {
      button_clicked = true;
      down_pos = _pos;
      return true;
    }
    else if (_type == MMOVE && button_clicked) {
      vec2 offset {_pos - down_pos};

      camera_rot += offset / vec2(w.size()) * float(M_PI);
      constexpr float max_y = M_PI_2 - 0.01;
      camera_rot.y = clamp(camera_rot.y, -max_y, max_y);
      down_pos = _pos;

      w.invalidate(false);
    }

    vec3 orbit_base = vec3(0, 0, 5);

    mat4 mrot = mat4(1);
    mrot = rotate(mrot, -camera_rot.x, vec3{0, 1, 0});
    mrot = rotate(mrot, -camera_rot.y, vec3{1, 0, 0});
    vec3 orbit = mrot * vec4(orbit_base, 1);

    mat4 m = lookAt(
        orbit,
        vec3(0, 0, 0),
        vec3{0, -1, 0});
    ubo->update_subone(0, offsetof(vp_ubo, view_), sizeof(mat4), &m);

    w.invalidate(false);

    return true;
  });

  return d.dispatch();
}
