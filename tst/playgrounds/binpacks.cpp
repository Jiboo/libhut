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

#include "hut/utils/binpacks.hpp"

#include "hut/display.hpp"
#include "hut/window.hpp"

#include "hut/imgui/imgui.hpp"

#include "tst_events.hpp"

using namespace hut;

template<typename T>
void debug(const binpack::linear1d<T> &_packer) {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2      p         = ImGui::GetCursorScreenPos();
  _packer.visit_blocks([p, draw_list](const auto &block) {
    if (!block.used_) {
      u16vec4 rect{block.offset_, 0, block.offset_ + block.size_, 100};
      draw_list->AddRectFilled(ImVec2(p.x + rect.x, p.y + rect.y), ImVec2(p.x + rect.z, p.y + rect.w), IM_COL32(0, 255, 0, 63), 0.0f, ImDrawCornerFlags_All);
      draw_list->AddRect(ImVec2(p.x + rect.x, p.y + rect.y), ImVec2(p.x + rect.z, p.y + rect.w), IM_COL32(0, 255, 0, 127), 0.0f, ImDrawCornerFlags_All);
    }
    return true;
  });
}

template<typename T, typename TUnderlying>
void debug(const binpack::adaptor1d_dummy2d<T, TUnderlying> &_packer) {
  debug(_packer.underlying_);
}

template<typename T, typename TShelveSelector>
void debug(const binpack::shelve<T, TShelveSelector> &_packer) {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2      p         = ImGui::GetCursorScreenPos();

  for (auto &row : _packer.rows_) {
    draw_list->AddText(ImVec2(p.x, p.y + row.second.y_), IM_COL32(255, 255, 255, 127), std::to_string(row.first).c_str());
  }
}

template<typename TPacker>
void ImPacker(const char *_name = "Packer", u16vec2 _bounds = {8 * 1024, 8 * 1024}, u16vec2 _min_rect = {5, 5}, u16vec2 _max_rect = {100, 100}) {
  ImGui::SetNextWindowContentSize({float(_bounds.x), float(_bounds.y)});
  if (ImGui::Begin(_name, nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {
    using clock_t = std::chrono::high_resolution_clock;
    using dur_t   = std::chrono::duration<float, std::milli>;
    static TPacker              packer(_bounds);
    static std::vector<u16vec4> packed;
    static std::mt19937         gen;
    static float                surface = 0;
    static std::vector<float>   add_timings;
    static float                total_add_time = 0;
    static std::vector<float>   rem_timings;
    static float                total_rem_time = 0;
    static auto                 padding        = ivec2{0, 0};

    auto rand_norm = [&]() {
      std::uniform_real_distribution<> dis(0.0, 1.0);
      return dis(gen);
    };
    auto rand_int = [&](int _end) {
      std::uniform_int_distribution dis(0, _end - 1);
      return dis(gen);
    };
    auto rand_size = [&]() {
      return u16vec2{
          (_max_rect.x - _min_rect.x) * rand_norm() + _min_rect.x,
          (_max_rect.y - _min_rect.y) * rand_norm() + _min_rect.y};
    };
    auto reset = [&]() {
      gen.seed(std::mt19937::default_seed);
      packed.clear();
      add_timings.clear();
      rem_timings.clear();
      packer.reset();
      surface        = 0;
      total_add_time = 0;
      total_rem_time = 0;
    };
    auto add_one = [&]() {
      auto rect_size = rand_size();
      auto before    = clock_t::now();
      auto rect      = packer.pack(rect_size + u16vec2(padding));
      auto after     = clock_t::now();
      auto time      = std::chrono::duration_cast<dur_t>(after - before).count();
      total_add_time += time;
      add_timings.emplace_back(time);
      if (rect) {
        surface += rect_size.x * rect_size.y;
        packed.emplace_back(make_bbox_with_origin_size(rect.value(), rect_size - u16vec2(padding)));
      }
      return rect.has_value();
    };
    auto remove_rand = [&]() {
      auto index     = rand_int(packed.size());
      auto rect_size = bbox_size(packed[index]);
      auto before    = clock_t::now();
      packer.offer(packed[index]);
      auto after = clock_t::now();
      auto time  = std::chrono::duration_cast<dur_t>(after - before).count();
      total_rem_time += time;
      rem_timings.emplace_back(time);
      surface -= rect_size.x * rect_size.y;
      packed.erase(packed.begin() + index);
    };

    if (ImGui::Button("Reset")) { reset(); }
    ImGui::SameLine();
    if (ImGui::Button("Add")) { add_one(); }
    ImGui::SameLine();
    if (ImGui::Button("Add 1000")) {
      for (int i = 0; i < 1000; i++) add_one();
    }
    ImGui::SameLine();
    if (ImGui::Button("Rem")) { remove_rand(); }
    ImGui::SameLine();
    if (ImGui::Button("Fill")) {
      reset();
      while (add_one())
        ;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stress")) {
      while (!packed.empty()) remove_rand();
      while (add_one())
        ;
    }
    ImGui::SameLine();
    ImGui::InputInt2("Padding", &padding.x);
    ImGui::Text("Packed %zu/%zu (with %zu rem), %f%% surface, %fms total add (%fms/op), %fms total rem (%fms/op)",
                packed.size(), add_timings.size(), rem_timings.size(),
                surface / float(_bounds.x * _bounds.y) * 100,
                total_add_time, total_add_time / add_timings.size(),
                total_rem_time, total_rem_time / rem_timings.size());
    ImGui::PlotHistogram("Add", add_timings.data(), add_timings.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 200));
    ImGui::PlotHistogram("Rem", rem_timings.data(), rem_timings.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 200));

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2      p         = ImGui::GetCursorScreenPos();

    draw_list->AddRect(ImVec2(p.x, p.y), ImVec2(p.x + _bounds.x, p.y + _bounds.y), IM_COL32(255, 0, 0, 255), 0.0f, ImDrawCornerFlags_All, 1.0f);
    for (auto &rect : packed) {
      draw_list->AddRectFilled(ImVec2(p.x + rect.x, p.y + rect.y), ImVec2(p.x + rect.z, p.y + rect.w), IM_COL32(255, 255, 0, 127), 0.0f, ImDrawCornerFlags_All);
      draw_list->AddRect(ImVec2(p.x + rect.x, p.y + rect.y), ImVec2(p.x + rect.z, p.y + rect.w), IM_COL32(255, 255, 0, 255), 0.0f, ImDrawCornerFlags_All);
    }
    debug(packer);

    ImGui::Dummy(ImVec2(_bounds.x, _bounds.y));
  }
  ImGui::End();
}

int main(int, char **) {
  display dsp("hut demo");

  window win(dsp);
  win.clear_color({0, 0, 0, 1});
  win.title(u8"hut imgui demo");

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!ImGui_ImplHut_Init(&dsp, &win, true))
    return EXIT_FAILURE;
  install_test_events(dsp, win);

  win.on_draw.connect([&](VkCommandBuffer _buffer) {
    ImGui_ImplHut_NewFrame();
    ImGui::NewFrame();

    ImPacker<binpack::adaptor1d_dummy2d<u16, binpack::linear1d<u16>>>("linear1d", {32 * 1024, 100}, {5, 100});
    ImPacker<binpack::shelve<u16, binpack::shelve_separator_align<u16, 16>>>("shelves<align<16>>");
    ImPacker<binpack::shelve<u16, binpack::shelve_separator_pow<u16, 16>>>("shelves<pow<16>>");

    ImGui::Render();
    ImGui_ImplHut_RenderDrawData(_buffer, ImGui::GetDrawData());

    return false;
  });

  dsp.dispatch();

  ImGui_ImplHut_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
