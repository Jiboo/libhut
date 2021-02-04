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

#include "demo_shaders.hpp"

using namespace hut;

struct vp_ubo {
  mat4 proj_;
  mat4 view_;
};

namespace hut::details {
  struct rgb3d {
    using impl = pipeline<hut::details::rgb3d>;
    using ubo = vp_ubo;

    struct instance {
      mat4 transform_;
    };

    struct vertex {
      vec3 pos_;
      vec3 color_;
    };

    constexpr static uint max_descriptor_sets_ = 1;

    constexpr static std::array<VkDescriptorPoolSize, 1> descriptor_pools_{
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = max_descriptor_sets_},
    };

    constexpr static std::array<VkDescriptorSetLayoutBinding, 1> descriptor_bindings_{
        VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .pImmutableSamplers = nullptr},
    };

    static void fill_extra_descriptor_writes(impl::descriptor_write_context &) {
    }

    constexpr static std::array<VkVertexInputBindingDescription, 2> vertices_binding_ = {
        VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
    };

    constexpr static std::array<VkVertexInputAttributeDescription, 6> vertices_description_{
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(vertex, pos_)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(vertex, color_)},
        VkVertexInputAttributeDescription{.location = 2, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(0)},
        VkVertexInputAttributeDescription{.location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(1)},
        VkVertexInputAttributeDescription{.location = 4, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(2)},
        VkVertexInputAttributeDescription{.location = 5, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(3)},
    };

    constexpr static decltype(hut_shaders::rgb_frag_spv) &frag_bytecode_ = hut_shaders::rgb_frag_spv;
    constexpr static decltype(demo_shaders::rgb3d_vert_spv) &vert_bytecode_ = demo_shaders::rgb3d_vert_spv;
  };
}
using rgb3d = hut::details::rgb3d::impl;

int main(int, char **) {
  display d("hut demo3d");

  window w(d);
  w.clear_color({1, 1, 1, 1});
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
    lookAt(vec3{0, 0, -5}, vec3{0, 0, 0}, vec3{0, 1, 0}),
  };
  shared_ref<vp_ubo> ubo = d.alloc_ubo(b, default_ubo);

  auto rgb3d_pipeline = std::make_unique<rgb3d>(w);
  auto rgb3d_instances = b->allocate<rgb3d::instance>(1);
  auto rgb3d_vertices = b->allocate<rgb3d::vertex>(8);
  rgb3d_vertices->set({
    // back
    rgb3d::vertex{{-1, -1, 1}, {1, 0, 0}},
    rgb3d::vertex{{ 1, -1, 1}, {0, 1, 0}},
    rgb3d::vertex{{ 1,  1, 1}, {0, 0, 1}},
    rgb3d::vertex{{-1,  1, 1}, {1, 1, 1}},
    // front
    rgb3d::vertex{{-1, -1, -1}, {1, 1, 1}},
    rgb3d::vertex{{ 1, -1, -1}, {0, 0, 1}},
    rgb3d::vertex{{ 1,  1, -1}, {0, 1, 0}},
    rgb3d::vertex{{-1,  1, -1}, {1, 0, 0}},
  });
  rgb3d_instances->set({
    rgb3d::instance{glm::identity<mat4>()},
  });
  rgb3d_pipeline->write(0, ubo);

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
    rgb3d_pipeline->draw(_buffer, 0, indices, rgb3d_instances, rgb3d_vertices);
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

  w.on_mouse.connect([&](uint8_t _button, mouse_event_type _type, vec2 _pos) {
    static bool button_clicked = false;
    static vec2 down_pos = {0, 0};

    if (_type == MUP) {
      button_clicked = false;

      mat4 m(1);
      rgb3d_instances->update_one(0, rgb3d::instance{m});
      w.invalidate(false);

      return true;
    }
    else if (_type == MDOWN) {
      button_clicked = true;
      down_pos = _pos;
      return true;
    }
    else if (_type == MMOVE && button_clicked) {
      vec2 offset {_pos - down_pos};

      mat4 m(1);
      m = rotate(m, std::numbers::pi_v<float> / 400 * offset.x, vec3{0, 1, 0});
      m = rotate(m, std::numbers::pi_v<float> / 400 * offset.y, vec3{1, 0, 0});
      rgb3d_instances->update_one(0, rgb3d::instance{m});

      w.invalidate(false);
    }

    return true;
  });

  return d.dispatch();
}
