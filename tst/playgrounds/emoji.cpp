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

#include "hut/atlas_pool.hpp"
#include "hut/display.hpp"
#include "hut/pipeline.hpp"
#include "hut/window.hpp"

#include "hut/imgui/imgui.hpp"
#include "hut/text/font.hpp"
#include "hut/text/shaping.hpp"

#include "tst_woff2.hpp"
#include "tst_events.hpp"
#include "tst_pipelines.hpp"

using namespace hut;

int main(int, char**) {
  display d("hut demo");
  auto b = d.alloc_buffer(1024*1024);

  window w(d, window_params{.size_{1024, 712}});
  w.clear_color({0, 0, 0, 0});
  w.title("hut imgui demo");

  shared_sampler samp = std::make_shared<sampler>(d);

  vec4 clear_color({0, 0, 0, 1});

  proj_ubo default_ubo;
  default_ubo.proj_ = ortho<float>(0, w.size().x, 0, w.size().y);
  shared_ref<proj_ubo> ubo = d.alloc_ubo(b, default_ubo);

  shared_atlas b8g8r8a8_atlas = std::make_shared<atlas_pool>(d, image_params{.size_ = {1024, 512}, .format_ = VK_FORMAT_B8G8R8A8_UNORM});
  auto pipeline = std::make_unique<atlas>(w);
  pipeline->write(0, ubo, b8g8r8a8_atlas, samp);

  auto text_instances = b->allocate<atlas::instance>(1);
  vec2 text_translate = {  1, 101};
  vec3 text_scale = {1,1,1};
  text_instances->set(atlas::instance{make_transform_mat4(text_translate, text_scale)});

  auto atlas_instances = b->allocate<atlas::instance>(1);
  auto atlas_vertices = b->allocate<atlas::vertex>(4);
  auto atlas_indices = b->allocate<uint16_t>(6);
  atlas_indices->set({0, 1, 2, 2, 1, 3});
  atlas_vertices->set({
    atlas::vertex{{0, 0}, {0, 0}},
    atlas::vertex{{0, 1}, {0, 1}},
    atlas::vertex{{1, 0}, {1, 0}},
    atlas::vertex{{1, 1}, {1, 1}},
  });
  atlas_instances->set(atlas::instance{make_transform_mat4({0, 200}, {b8g8r8a8_atlas->size(), 1})});

  std::vector<shared_font> fonts {
    std::make_shared<font>(d, tst_woff2::TwemojiMozilla_woff2.data(), tst_woff2::TwemojiMozilla_woff2.size(), b8g8r8a8_atlas, true),
  };
  std::vector<const char*> font_names {
    "TwemojiMozilla_woff2",
  };
  int font_selection = 0;
  int atlas_page = 0;

  using myshaper = shaper<uint16_t, atlas::vertex>;
  myshaper s;
  myshaper::result sresult;

  float font_size = 64;

  char32_t c32_emoji = U'😀';
  char8_t c8_emoji[6];

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!ImGui_ImplHut_Init(&d, &w, true))
    return EXIT_FAILURE;
  install_test_events(d, w, ubo);

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
    pipeline->update_atlas(0, b8g8r8a8_atlas);

    ImGui_ImplHut_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Hello, world!")) {
      ImGui::ColorEdit4("clear color", (float*)&clear_color); // Edit 3 floats representing a color
      ImGui::InputFloat4("translate,", &text_translate[0]);
      ImGui::InputFloat3("scale,", &text_scale[0]);
      ImGui::InputFloat("font size,", &font_size);
      ImGui::Combo("font", &font_selection, font_names.data(), font_names.size());
      ImGui::SliderInt("atlas page", &atlas_page, 0, b8g8r8a8_atlas->page_count() - 1);

      ImGui::InputScalar("emoji utf32 code", ImGuiDataType_U32, &c32_emoji, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
      ImGui::DragInt("utf32", (int*)&c32_emoji, 1, 0x1f600, 0x1fFFF);
      *to_utf8(c8_emoji, c32_emoji) = '\0';

      myshaper::context ctx {b, sresult, fonts[font_selection], (uint8_t)font_size, (char*)c8_emoji};

      s.bake(ctx);
      pipeline->draw(_buffer, 0, sresult.indices_, sresult.indices_count_, text_instances, sresult.vertices_);
      pipeline->draw(_buffer, 0, atlas_indices, atlas_instances, atlas_vertices);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplHut_RenderDrawData(_buffer, ImGui::GetDrawData());

    return false;
  });

  w.on_frame.connect([&](display::duration _dt) {
    d.post([&](auto){
      w.invalidate(true);
      w.clear_color(clear_color);
      text_instances->set(atlas::instance{make_transform_mat4(text_translate, text_scale)});

      atlas_vertices->set({
        atlas::vertex{{0, 0}, encode_atlas_page_xy(vec2{0, 0}, atlas_page)},
        atlas::vertex{{0, 1}, encode_atlas_page_xy(vec2{0, 1}, atlas_page)},
        atlas::vertex{{1, 0}, encode_atlas_page_xy(vec2{1, 0}, atlas_page)},
        atlas::vertex{{1, 1}, encode_atlas_page_xy(vec2{1, 1}, atlas_page)},
      });
    });

    return false;
  });

  d.dispatch();

  ImGui_ImplHut_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
