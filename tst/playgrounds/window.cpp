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

#include <fstream>
#include <iostream>
#include <random>

#include "hut/display.hpp"
#include "hut/window.hpp"

#include "hut/imgui/imgui.hpp"

#include "tst_events.hpp"

using namespace hut;

std::shared_ptr<window> create_window(display &_display, window_params _params, std::string_view _title, vec4 _clearc) {
  auto result = std::make_shared<window>(_display, _params);
  result->title(_title);
  result->clear_color(_clearc);
  install_test_events(_display, *result);

  return result;
}

const char *window_params_flag_name(window_params::flag _flag) {
  switch (_flag) {
    case window_params::FDEPTH: return "FDEPTH";
    case window_params::FMULTISAMPLING: return "FMULTISAMPLING";
    case window_params::FVSYNC: return "FVSYNC";
    case window_params::FTRANSPARENT: return "FTRANSPARENT";
    case window_params::FFULLSCREEN: return "FFULLSCREEN";
  }
  assert(false);
  return "invalid";
}

int main(int, char**) {
  display d("hut demo");

  window_params wp;
  wp.flags_.set(window_params::FTRANSPARENT);
  window w(d, wp);
  w.title("hut imgui demo");
  w.clear_color({0, 0, 0, 1});

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!ImGui_ImplHut_Init(&d, &w, true))
    return EXIT_FAILURE;
  install_test_events(d, w);

  int current_corsor = CDEFAULT;
  bool set_maximize = true;
  bool set_fullscreen = true;
  char title[1024] = "Hello world";
  vec4 clear_color {0, 0, 0, 1};

  window_params params;
  char create_title[1024] = "Hello world";
  vec4 create_clear_color {0, 0, 0, 1};
  std::vector<std::shared_ptr<window>> windows;

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
    ImGui_ImplHut_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Window playground")) {
      ImGui::TextUnformatted("Drag border for interactive resize, drag window for interactive move");

      ImGui::Checkbox("Set##max", &set_maximize);
      ImGui::SameLine();
      if (ImGui::Button("Maximize")) {
        w.maximize(set_maximize);
        set_maximize = !set_maximize;
      }

      ImGui::Checkbox("Set##full", &set_fullscreen);
      ImGui::SameLine();
      if (ImGui::Button("Fullscreen")) {
        w.fullscreen(set_fullscreen);
        set_fullscreen = !set_fullscreen;
      }

      if (ImGui::InputText("Title", title, sizeof(title)))
        w.title(title);

      if (ImGui::ColorEdit4("Clear color", &clear_color.x))
        w.clear_color(clear_color);

      auto item_getter = [](void*, int _cursor, const char** _dst) {
        *_dst = cursor_css_name(static_cast<cursor_type>(_cursor));
        return true;
      };
      if (ImGui::Combo("Cursor", &current_corsor, item_getter, nullptr, CURSOR_TYPE_LAST_VALUE)) {
        w.cursor(static_cast<cursor_type>(current_corsor));
      }
    }
    ImGui::End();

    if (ImGui::Begin("Create window")) {
      ImGui_HutFlag("flags", &params.flags_, window_params_flag_name);
      ImGui::InputScalarN("position", ImGuiDataType_U32, &params.size_, 2);
      ImGui::InputScalarN("min_size", ImGuiDataType_U32, &params.min_size_, 2);
      ImGui::InputScalarN("max_size", ImGuiDataType_U32, &params.max_size_, 2);
      ImGui::InputText("Title", create_title, sizeof(create_title));
      ImGui::ColorEdit4("Clear color", &create_clear_color.x);

      if (ImGui::Button("create"))
        windows.emplace_back(create_window(d, params, create_title, create_clear_color));
      if (ImGui::Button("close all"))
        d.post([&](auto){ windows.clear(); });
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplHut_RenderDrawData(_buffer, ImGui::GetDrawData());
    return false;
  });

  d.dispatch();

  ImGui_ImplHut_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
