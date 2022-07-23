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

#include "hut/atlas.hpp"
#include "hut/display.hpp"
#include "hut/window.hpp"

#include "hut/imgdec/imgdec.hpp"
#include "hut/imgui/imgui.hpp"
#include "hut/render2d/renderer.hpp"

#include "tst_events.hpp"
#include "tst_png.hpp"
#include "tst_utils.hpp"

using namespace hut;

int main(int /*unused*/, char ** /*unused*/) {
  display       dsp("hut render2d playground");
  shared_buffer buf = std::make_shared<buffer>(dsp);

  window win(dsp, buf, window_params{.flags_{window_params::FTRANSPARENT, window_params::FVSYNC}});
  win.title(u8"hut render2d playground");
  win.clear_color({0, 0, 0, 1});

  auto texatlas = std::make_shared<atlas>(
      dsp, buf, image_params{.size_ = dsp.max_tex_size(), .format_ = VK_FORMAT_R8G8B8A8_UNORM});
  auto samp = std::make_shared<sampler>(dsp);
  auto tex  = imgdec::load_png(texatlas, tst_png::tex1_png);
  auto ubo  = buf->allocate<common_ubo>(1, dsp.ubo_align());
  ubo->set(common_ubo{win.size()});

  render2d::renderer box_renderer(win, buf, ubo, texatlas, samp);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!imgui::init(&dsp, &win, buf, true))
    return EXIT_FAILURE;
  install_test_events(dsp, win, ubo);

  auto quads = box_renderer.allocate(1);

  u16_px       bbox_min        = 0_px;
  u16_px       bbox_max        = 1000_px;
  u16bbox_px   custom_bbox     = {0, 0, 0, 0};
  u8vec4_rgba  custom_col_from = {255, 0, 0, 255};
  u8vec4_rgba  custom_col_to   = {0, 0, 255, 255};
  f32vec4_rgba clear_color     = {0, 0, 0, 1};

  u16  custom_radius   = 8;
  u16  custom_softness = 1;
  u16  extra_min       = 0;
  u16  extra_max       = 15;
  int  custom_gradient = 0;
  int  custom_mode     = 0;
  bool custom_use_tex  = false;

  bool grid_use_tex                = false;
  bool grid_fixed_colors           = false;
  bool grid_fixed_gradient         = false;
  bool grid_fixed_mode             = true;
  bool grid_release_before_realloc = false;
  bool grid_no_params              = false;
  bool grid_rand_transparency      = false;

  u16vec2_px grid_size  = {50, 50};
  u16_px     grid_min   = 10_px;
  u16_px     grid_max   = 200_px;
  f32_px     scroll_min = -800._px;
  f32_px     scroll_max = 800_px;
  vec2_px    scroll     = {0, 0};

  win.on_draw_.connect([&](VkCommandBuffer _buffer) {
    imgui::new_frame();
    ImGui::NewFrame();

    if (ImGui::Begin("renderer2d")) {
      tst::rand::reset();

      if (ImGui::ColorEdit4("Window clear color", &clear_color.x))
        win.clear_color(clear_color);

      ImGui::Separator();

      u16vec2_px origin = custom_bbox.origin();
      u16vec2_px size   = custom_bbox.size();
      ImGui::SliderScalarN("Origin", ImGuiDataType_U16, &origin, 2, &bbox_min, &bbox_max);
      ImGui::SliderScalarN("Size", ImGuiDataType_U16, &size, 2, &bbox_min, &bbox_max);
      custom_bbox = u16bbox_px::with_origin_size(origin, size);
      {
        f32vec4_rgba color_buff = custom_col_from;
        ImGui::ColorEdit4("From", &color_buff.r);
        custom_col_from = color_buff;
      }
      {
        f32vec4_rgba color_buff = custom_col_to;
        ImGui::ColorEdit4("To", &color_buff.r);
        custom_col_to = color_buff;
      }
      ImGui::SliderScalar("Corner radius", ImGuiDataType_U16, &custom_radius, &extra_min, &extra_max);
      ImGui::SliderScalar("Corner softness", ImGuiDataType_U16, &custom_softness, &extra_min, &extra_max);
      constexpr const char *GRADIENTS[] = {"T2B", "L2R", "TL2BT", "TR2BL"};
      ImGui::Combo("Gradient", &custom_gradient, GRADIENTS, 4);
      ImGui::Checkbox("Use texture", &custom_use_tex);
      constexpr const char *MODES[] = {"ROUNDED", "BORDER", "SHADOW"};
      ImGui::Combo("Render mode", &custom_mode, MODES, 3);

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
      ImGui::Checkbox("Grid render mode", &grid_fixed_mode);
      ImGui::Checkbox("Grid fixed colors", &grid_fixed_colors);
      ImGui::Checkbox("Grid fixed gradient", &grid_fixed_gradient);
      ImGui::Checkbox("Grid no feather/corner", &grid_no_params);
      ImGui::Checkbox("Grid random transparency", &grid_rand_transparency);

      auto iupdator = quads.update();
      u16  scale    = win.scale();
      for (unsigned i = 0; i < quads.size() - 1; i++) {
        auto             line    = i / 15;
        auto             col     = i % 15;
        constexpr u16_px PADDING = 8_px;
        auto             offset  = u16vec2_px{(grid_size.x + PADDING) * col, (grid_size.y + PADDING) * line};
        u16bbox_px       bbox    = u16bbox_px::with_origin_size(u16vec2_px{PADDING} + offset, grid_size);
        render2d::set(
            iupdator[i],
            render2d::box_params{
                .bbox_          = bbox * scale,
                .from_          = grid_fixed_colors ? custom_col_from : tst::rand::color(grid_rand_transparency ? .2 : 1),
                .to_            = grid_fixed_colors ? custom_col_to : tst::rand::color(grid_rand_transparency ? .2 : 1),
                .gradient_      = grid_fixed_gradient ? render2d::gradient(custom_gradient) : render2d::gradient(i % 4),
                .mode_          = grid_fixed_mode ? render2d::mode(custom_mode) : render2d::mode(i % 3),
                .corner_radius_ = grid_no_params ? 0 : col,
                .corner_softness_ = grid_no_params ? 0 : line,
                .subimg_          = grid_use_tex ? tex.get() : nullptr,
            });
      }
      render2d::set(iupdator[quads.size() - 1], render2d::box_params{
                                                    .bbox_            = custom_bbox * scale,
                                                    .from_            = custom_col_from,
                                                    .to_              = custom_col_to,
                                                    .gradient_        = render2d::gradient(custom_gradient),
                                                    .mode_            = render2d::mode(custom_mode),
                                                    .corner_radius_   = custom_radius,
                                                    .corner_softness_ = custom_softness,
                                                    .subimg_          = custom_use_tex ? tex.get() : nullptr,
                                                });

      ImGui::Separator();

      if (ImGui::SliderScalarN("Scroll", ImGuiDataType_Float, &scroll, 2, &scroll_min, &scroll_max)) {
        mat4 translate = make_transform_mat4(scroll);
        ubo->set_subone(0, offsetof(common_ubo, view_), sizeof(mat4), &translate);
      }

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                  ImGui::GetIO().Framerate);
    }
    ImGui::End();

    box_renderer.draw(_buffer);

    ImGui::Render();
    imgui::render(_buffer, ImGui::GetDrawData());
    return false;
  });
  win.on_frame_.connect([&](display::duration _dt) {
    dsp.post([&](auto) { win.invalidate(true); });
    return false;
  });

  dsp.dispatch();

  imgui::shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
