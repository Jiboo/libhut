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

#include "hut/display.hpp"
#include "hut/atlas_pool.hpp"
#include "hut/font.hpp"
#include "hut/pipeline.hpp"
#include "hut/shaping.hpp"
#include "hut/window.hpp"

#include "hut_imgui.hpp"
#include "tst_woff2.hpp"
#include "tst_events.hpp"

using namespace hut;

int main(int, char**) {
  display d("hut demo");

  window w(d);
  w.clear_color({0, 0, 0, 1});
  w.title("hut imgui demo");

  shared_sampler samp = std::make_shared<sampler>(d);
  auto b = d.alloc_buffer(1024*1024);

  vec4 clear_color({0, 0, 0, 1});

  proj_ubo default_ubo;
  default_ubo.proj_ = ortho<float>(0, w.size().x, 0, w.size().y);
  shared_ref<proj_ubo> ubo = d.alloc_ubo(b, default_ubo);

  shared_atlas atlas = std::make_shared<atlas_pool>(d, image_params{.size_ = {1024, 1024}, .format_ = VK_FORMAT_R8_UNORM});
  auto pipeline = std::make_unique<atlas_mask>(w);
  pipeline->write(0, ubo, atlas, samp);

  auto text_instances = b->allocate<atlas_mask::instance>(1);
  vec2 text_translate = {  1, 101};
  vec3 text_scale = {1,1,1};
  vec4 text_color = {1, 1, 1, 1};
  text_instances->set(atlas_mask::instance{make_transform_mat4(text_translate, text_scale), text_color});

  auto atlas_instances = b->allocate<atlas_mask::instance>(1);
  auto atlas_vertices = b->allocate<atlas_mask::vertex>(4);
  auto atlas_indices = b->allocate<uint16_t>(6);
  atlas_indices->set({0, 1, 2, 2, 1, 3});
  atlas_vertices->set({
    atlas_mask::vertex{{0, 0}, {0, 0}},
    atlas_mask::vertex{{0, 1}, {0, 1}},
    atlas_mask::vertex{{1, 0}, {1, 0}},
    atlas_mask::vertex{{1, 1}, {1, 1}},
  });
  atlas_instances->set(atlas_mask::instance{make_transform_mat4({0, 200}, {atlas->image(0)->size(), 1}), {1, 1, 1, 1}});

  std::vector<shared_font> fonts {
    std::make_shared<font>(d, tst_woff2::Roboto_Regular_woff2.data(), tst_woff2::Roboto_Regular_woff2.size(), atlas, true),
    std::make_shared<font>(d, tst_woff2::Roboto_Medium_woff2.data(), tst_woff2::Roboto_Medium_woff2.size(), atlas, true),
    std::make_shared<font>(d, tst_woff2::Roboto_Thin_woff2.data(), tst_woff2::Roboto_Thin_woff2.size(), atlas, true),
    std::make_shared<font>(d, tst_woff2::Roboto_Light_woff2.data(), tst_woff2::Roboto_Light_woff2.size(), atlas, true),
    std::make_shared<font>(d, tst_woff2::Roboto_Italic_woff2.data(), tst_woff2::Roboto_Italic_woff2.size(), atlas, true),
    std::make_shared<font>(d, tst_woff2::Roboto_Bold_woff2.data(), tst_woff2::Roboto_Bold_woff2.size(), atlas, true),
    std::make_shared<font>(d, tst_woff2::Roboto_BoldItalic_woff2.data(), tst_woff2::Roboto_BoldItalic_woff2.size(), atlas, true),
    std::make_shared<font>(d, tst_woff2::ProggyClean_woff2.data(), tst_woff2::ProggyClean_woff2.size(), atlas, true),
    std::make_shared<font>(d, tst_woff2::materialdesignicons_webfont_woff2.data(), tst_woff2::materialdesignicons_webfont_woff2.size(), atlas, true),
  };
  std::vector<const char*> font_names {
    "Roboto_Regular_woff2",
    "Roboto_Medium_woff2",
    "Roboto_Thin_woff2",
    "Roboto_Light_woff2",
    "Roboto_Italic_woff2",
    "Roboto_Bold_woff2",
    "Roboto_BoldItalic_woff2",
    "ProggyClean_woff2",
    "materialdesignicons_webfont_woff2",
  };
  int font_selection = 0;
  int atlas_page = 0;

  using myshaper = shaper<uint16_t, atlas_mask::vertex>;
  myshaper s;
  myshaper::result sresult;

  float font_size = 12;
  static char text[1024 * 16] =
      "Validation Error: [ VUID-vkCmdDrawIndexed-None-04007 ] Object 0: handle = 0x1116d20,"
      "type = VK_OBJECT_TYPE_COMMAND_BUFFER; | MessageID = 0x5e4491cd | vkCmdDrawIndexed():"
      "VkPipeline 0x290000000029[] expects that this Command Buffer's vertex binding Index 1"
      "should be set via vkCmdBindVertexBuffers. This is because VkVertexInputBindingDescription"
      "struct at index 1 of pVertexBindingDescriptions has a binding value of 1."
      "The Vulkan spec states: All vertex input bindings accessed via vertex input variables"
      "declared in the vertex shader entry point's interface must have either valid or VK_NULL_HANDLE"
      "buffers bound (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VUID-vkCmdDrawIndexed-None-04007)";

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!ImGui_ImplHut_Init(&d, &w, true))
    return EXIT_FAILURE;
  install_test_events(d, w, ubo);

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
    pipeline->update_atlas(0, atlas);
    ImGui_ImplHut_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Hello, world!")) {
      ImGui::ColorEdit4("clear color", (float*)&clear_color); // Edit 3 floats representing a color
      ImGui::ColorEdit4("text color", (float*)&text_color); // Edit 3 floats representing a color
      ImGui::InputFloat4("translate,", &text_translate[0]);
      ImGui::InputFloat3("scale,", &text_scale[0]);
      ImGui::InputFloat("font size,", &font_size);
      ImGui::Combo("font", &font_selection, font_names.data(), font_names.size());
      ImGui::SliderInt("atlas page", &atlas_page, 0, atlas->page_count() - 1);

      static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
      ImGui::InputTextMultiline("##source", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);

      myshaper::context ctx{b, sresult, fonts[font_selection], (u8)font_size, text};
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
      text_instances->set(atlas_mask::instance{make_transform_mat4(text_translate, text_scale), text_color});

      atlas_vertices->set({
        atlas_mask::vertex{{0, 0}, encode_atlas_page_xy(vec2{0, 0}, atlas_page)},
        atlas_mask::vertex{{0, 1}, encode_atlas_page_xy(vec2{0, 1}, atlas_page)},
        atlas_mask::vertex{{1, 0}, encode_atlas_page_xy(vec2{1, 0}, atlas_page)},
        atlas_mask::vertex{{1, 1}, encode_atlas_page_xy(vec2{1, 1}, atlas_page)},
      });
    });

    return false;
  });

  d.dispatch();

  ImGui_ImplHut_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
