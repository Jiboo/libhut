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

#include <glm/gtx/color_space.hpp>

#include "hut/atlas.hpp"
#include "hut/display.hpp"
#include "hut/window.hpp"

#include "hut/imgdec/imgdec.hpp"
#include "hut/imgui/imgui.hpp"
#include "hut/render2d/renderer.hpp"

#include "tst_events.hpp"
#include "tst_png.hpp"

using namespace hut;

int main(int, char **) {
  display dsp("hut render2d playground");
  auto    buf = std::make_shared<buffer>(dsp, 256 * 1024 * 1024);

  window win(dsp);
  win.title(u8"hut render2d playground");
  win.clear_color({1, 1, 1, 1});

  auto texatlas
      = std::make_shared<atlas>(dsp, image_params{.size_ = dsp.max_tex_size(), .format_ = VK_FORMAT_R8G8B8A8_UNORM});
  auto samp = std::make_shared<sampler>(dsp);
  auto tex  = imgdec::load_png(texatlas, tst_png::tex1_png);
  auto ubo  = buf->allocate<common_ubo>(1, dsp.ubo_align());
  ubo->set(common_ubo{win.size()});

  render2d::renderer box_renderer(win, buf, ubo, texatlas, samp);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!ImGui_ImplHut_Init(&dsp, &win, true))
    return EXIT_FAILURE;
  install_test_events<common_ubo>(dsp, win, ubo);

  std::mt19937 gen;
  auto         rand_norm = [&]() {
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
  };

  auto rand_color = [&]() {
    auto hsv  = vec3{rand_norm() * 360, 0.99, 0.95};
    auto rgba = vec4(rgbColor(hsv), rand_norm() * 0.75 + 0.25);
    return u8vec4(rgba * 255);
  };

  auto quads = box_renderer.allocate(15 * 15 + 1);

  u16     bbox_min = 0, bbox_max = 1000;
  u16vec4 custom_bbox     = {0, 0, 0, 0};
  u8vec4  custom_col_from = {255, 0, 0, 255}, custom_col_to = {0, 0, 255, 255};
  u16     custom_radius = 8, custom_softness = 1;
  u16     extra_min = 0, extra_max = 15;
  int     custom_gradient = 0;
  bool    custom_use_tex  = false;

  bool    grid_use_tex                = false;
  bool    grid_fixed_colors           = false;
  bool    grid_fixed_gradient         = false;
  bool    grid_release_before_realloc = false;
  bool    grid_no_params              = false;
  u16vec2 grid_size                   = {75, 75};
  u16     grid_min = 10, grid_max = 200;
  float   scroll_min = -800, scroll_max = 800;
  vec2    scroll = {0, 0};

  win.on_draw.connect([&](VkCommandBuffer _buffer) {
    ImGui_ImplHut_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("renderer2d")) {
      gen.seed(std::mt19937::default_seed);

      u16vec2 origin = bbox_origin(custom_bbox), size = bbox_size(custom_bbox);
      ImGui::SliderScalarN("Origin", ImGuiDataType_U16, &origin, 2, &bbox_min, &bbox_max);
      ImGui::SliderScalarN("Size", ImGuiDataType_U16, &size, 2, &bbox_min, &bbox_max);
      custom_bbox = make_bbox_with_origin_size(origin, size);
      {
        float color_buff[4] = {custom_col_from.r / 255.f, custom_col_from.g / 255.f, custom_col_from.b / 255.f,
                               custom_col_from.a / 255.f};
        ImGui::ColorEdit4("From", color_buff);
        custom_col_from = u16vec4{color_buff[0] * 255, color_buff[1] * 255, color_buff[2] * 255, color_buff[3] * 255};
      }
      {
        float color_buff[4]
            = {custom_col_to.r / 255.f, custom_col_to.g / 255.f, custom_col_to.b / 255.f, custom_col_to.a / 255.f};
        ImGui::ColorEdit4("To", color_buff);
        custom_col_to = u16vec4{color_buff[0] * 255, color_buff[1] * 255, color_buff[2] * 255, color_buff[3] * 255};
      }
      ImGui::SliderScalar("Corner radius", ImGuiDataType_U16, &custom_radius, &extra_min, &extra_max);
      ImGui::SliderScalar("Corner softness", ImGuiDataType_U16, &custom_softness, &extra_min, &extra_max);
      constexpr const char *GRADIENTS[] = {"T2B", "L2R", "TL2BT", "TR2BL"};
      ImGui::Combo("Gradient", &custom_gradient, GRADIENTS, 4);
      ImGui::Checkbox("Use texture", &custom_use_tex);

      ImGui::Separator();

      if (ImGui::Button("Realloc grid")) {
        if (grid_release_before_realloc)
          quads.release();
        quads = box_renderer.allocate(15 * 15 + 1);
      }
      ImGui::SameLine();
      ImGui::Checkbox("Free before", &grid_release_before_realloc);
      ImGui::SameLine();
      ImGui::Text("Batch %p, Offset: %d, size: %d", quads.parent(), quads.offset_bytes(), quads.size_bytes());

      ImGui::SliderScalarN("Grid size", ImGuiDataType_U16, &grid_size, 2, &grid_min, &grid_max);
      ImGui::Checkbox("Grid use texture", &grid_use_tex);
      ImGui::Checkbox("Grid fixed colors", &grid_fixed_colors);
      ImGui::Checkbox("Grid fixed gradient", &grid_fixed_gradient);
      ImGui::Checkbox("Grid no feather/corner", &grid_no_params);

      auto iupdator = quads.update();
      u16  scale    = win.scale();
      for (unsigned i = 0; i < quads.size() - 1; i++) {
        auto    line = i / 15;
        auto    col  = i % 15;
        u16vec4 bbox = make_bbox_with_origin_size(
            u16vec2{8} + u16vec2{col * (grid_size.x + 8), line * (grid_size.y + 8)}, grid_size);
        render2d::set(iupdator[i], bbox * scale, grid_fixed_colors ? custom_col_from : rand_color(),
                      grid_fixed_colors ? custom_col_to : rand_color(),
                      grid_fixed_gradient ? render2d::gradient(custom_gradient) : render2d::gradient(i % 4),
                      grid_no_params ? 0 : col, grid_no_params ? 0 : line, grid_use_tex ? tex : shared_subimage{});
      }
      render2d::set(iupdator[quads.size() - 1], custom_bbox * scale, custom_col_from, custom_col_to,
                    render2d::gradient(custom_gradient), custom_radius, custom_softness,
                    custom_use_tex ? tex : shared_subimage{});

      ImGui::Separator();

      if (ImGui::SliderScalarN("Scroll", ImGuiDataType_Float, &scroll, 2, &scroll_min, &scroll_max)) {
        mat4 translate = make_transform_mat4(scroll);
        ubo->set_subone(0, offsetof(common_ubo, view_), sizeof(hut::mat4), &translate);
      }

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                  ImGui::GetIO().Framerate);
    }
    ImGui::End();

    box_renderer.draw(_buffer);

    ImGui::Render();
    ImGui_ImplHut_RenderDrawData(_buffer, ImGui::GetDrawData());
    return false;
  });
  win.on_frame.connect([&](display::duration _dt) {
    dsp.post([&](auto) { win.invalidate(true); });
    return false;
  });

  dsp.dispatch();

  ImGui_ImplHut_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
