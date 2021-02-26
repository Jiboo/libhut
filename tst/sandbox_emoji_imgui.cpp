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

#include <random>

#include "imgui_impl_hut.h"
#include "demo_woff2.hpp"

#include "hut/atlas_pool.hpp"
#include "hut/display.hpp"
#include "hut/font.hpp"
#include "hut/pipeline.hpp"
#include "hut/shaping.hpp"
#include "hut/window.hpp"

using namespace hut;

int main(int, char**) {
  hut::display d("hut demo");

  hut::window w(d, window_params {.position_ = {0, 0, 1920, 1080}});
  w.clear_color({1, 1, 1, 1});
  w.title("hut imgui demo");

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig font_cfg{};
  font_cfg.FontDataOwnedByAtlas = false;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Setup Platform/Renderer backends
  if (!ImGui_ImplHut_Init(&d, &w, true))
    return EXIT_FAILURE;

  shared_sampler samp = std::make_shared<sampler>(d);
  auto b = d.alloc_buffer(1024*1024);

  vec4 clear_color({0, 0, 0, 1});

  proj_ubo default_ubo;
  default_ubo.proj_ = ortho<float>(0, w.size().x, 0, w.size().y);
  shared_ref<proj_ubo> ubo = d.alloc_ubo(b, default_ubo);

  shared_atlas b8g8r8a8_atlas = std::make_shared<atlas_pool>(d, image_params{.size_ = {1024, 1024}, .format_ = VK_FORMAT_B8G8R8A8_UNORM});
  auto pipeline = std::make_unique<tex>(w);
  pipeline->write(0, ubo, b8g8r8a8_atlas->image(0), samp);

  auto text_instances = b->allocate<tex::instance>(1);
  vec2 text_translate = {  1, 101};
  vec3 text_scale = {1,1,1};
  text_instances->set(tex::instance{make_transform_mat4(text_translate, text_scale)});

  auto atlas_instances = b->allocate<tex::instance>(1);
  auto atlas_vertices = b->allocate<tex::vertex>(4);
  auto atlas_indices = b->allocate<uint16_t>(6);
  atlas_indices->set({0, 1, 2, 2, 1, 3});
  atlas_vertices->set({
    tex::vertex{{0, 0}, {0, 0}},
    tex::vertex{{0, 1}, {0, 1}},
    tex::vertex{{1, 0}, {1, 0}},
    tex::vertex{{1, 1}, {1, 1}},
  });
  atlas_instances->set(tex::instance{make_transform_mat4({0, 200}, {b8g8r8a8_atlas->image(0)->size(), 1})});

  std::vector<shared_font> fonts {
    std::make_shared<font>(d, demo_woff2::TwemojiMozilla_woff2.data(), demo_woff2::TwemojiMozilla_woff2.size(), b8g8r8a8_atlas, true),
    std::make_shared<font>(d, demo_woff2::materialdesignicons_webfont_woff2.data(), demo_woff2::materialdesignicons_webfont_woff2.size(), b8g8r8a8_atlas, true),
  };
  std::vector<const char*> font_names {
    "TwemojiMozilla_woff2",
    "materialdesignicons_webfont_woff2",
  };
  int font_selection = 0;

  using myshaper = shaper<uint16_t, tex::vertex>;
  myshaper s;
  myshaper::result sresult;

  float font_size = 64;

  char32_t c32_emoji = U'😀';
  char8_t c8_emoji[6];

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
    ImGui_ImplHut_NewFrame();
    ImGui::NewFrame();

    {
      ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

      ImGui::ColorEdit4("clear color", (float*)&clear_color); // Edit 3 floats representing a color
      ImGui::InputFloat4("translate,", &text_translate[0]);
      ImGui::InputFloat3("scale,", &text_scale[0]);
      ImGui::InputFloat("font size,", &font_size);
      ImGui::Combo("font", &font_selection, font_names.data(), font_names.size());

      ImGui::InputScalar("emoji utf32 code", ImGuiDataType_U32, &c32_emoji, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
      *to_utf8(c8_emoji, c32_emoji) = '\0';

      myshaper::context ctx {b, sresult, fonts[font_selection], (uint8_t)font_size, (char*)c8_emoji};

      s.bake(ctx);
      pipeline->draw(_buffer, 0, sresult.indices_, sresult.indices_count_, text_instances, sresult.vertices_);
      pipeline->draw(_buffer, 0, atlas_indices, atlas_instances, atlas_vertices);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplHut_RenderDrawData(_buffer, ImGui::GetDrawData());

    return false;
  });

  w.on_frame.connect([&](display::duration _dt) {
    d.post([&](auto){
      w.invalidate(true);
      w.clear_color(clear_color);
      text_instances->set(tex::instance{make_transform_mat4(text_translate, text_scale)});
    });

    return false;
  });

  w.on_key.connect([&](keycode _kcode, keysym _ksym, bool _down) {
    if (_ksym == KSYM_ESC)
      w.close();
    return false;
  });

  d.dispatch();

  ImGui_ImplHut_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
