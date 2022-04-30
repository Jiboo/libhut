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

template<typename TSizeType>
void debug(const binpack::linear1d<TSizeType> &_packer) {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2      p         = ImGui::GetCursorScreenPos();
  _packer.visit_blocks([p, draw_list](const auto &_block) {
    if (!_block.used_) {
      u16vec4 rect{_block.offset_, 0, _block.offset_ + _block.size_, 100};
      draw_list->AddRectFilled(ImVec2(p.x + rect.x, p.y + rect.y), ImVec2(p.x + rect.z, p.y + rect.w),
                               IM_COL32(0, 255, 0, 63));
      draw_list->AddRect(ImVec2(p.x + rect.x, p.y + rect.y), ImVec2(p.x + rect.z, p.y + rect.w),
                         IM_COL32(0, 255, 0, 127));
    }
    return true;
  });
}

template<typename TSizeType, typename TUnderlying>
void debug(const binpack::adaptor1d_dummy2d<TSizeType, TUnderlying> &_packer) {
  debug(_packer.underlying_);
}

template<typename TSizeType, typename TShelveSelector>
void debug(const binpack::shelve<TSizeType, TShelveSelector> &_packer) {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2      p         = ImGui::GetCursorScreenPos();

  for (auto &row : _packer.rows_) {
    draw_list->AddText(ImVec2(p.x, p.y + row.second.y_), IM_COL32(255, 255, 255, 127),
                       std::to_string(row.first).c_str());
  }
}

template<typename TPacker>
void packer(const char *_name = "Packer", u16vec2_px _bounds = {8 * 1024, 8 * 1024}, u16vec2_px _min_rect = {5, 5},
            u16vec2_px _max_rect = {100, 100}) {
  ImGui::SetNextWindowContentSize({float(_bounds.x), float(_bounds.y)});
  if (ImGui::Begin(_name, nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {
    using hrclock_t = std::chrono::high_resolution_clock;
    using dur_t     = std::chrono::duration<float, std::milli>;
    static TPacker              s_packer(_bounds);
    static std::vector<u16vec4> s_packed;
    static std::mt19937         s_gen;
    static float                s_surface = 0;
    static std::vector<float>   s_add_timings;
    static float                s_total_add_time = 0;
    static std::vector<float>   s_rem_timings;
    static float                s_total_rem_time = 0;
    static u16vec2_px           s_padding        = {0, 0};

    auto rand_norm = [&]() {
      std::uniform_real_distribution<> dis(0.0, 1.0);
      return dis(s_gen);
    };
    auto rand_int = [&](int _end) {
      std::uniform_int_distribution dis(0, _end - 1);
      return dis(s_gen);
    };
    auto rand_size = [&]() {
      return u16vec2_px{(_max_rect.x - _min_rect.x) * rand_norm() + _min_rect.x,
                        (_max_rect.y - _min_rect.y) * rand_norm() + _min_rect.y};
    };
    auto reset = [&]() {
      s_gen.seed(std::mt19937::default_seed);
      s_packed.clear();
      s_add_timings.clear();
      s_rem_timings.clear();
      s_packer.reset();
      s_surface        = 0;
      s_total_add_time = 0;
      s_total_rem_time = 0;
    };
    auto add_one = [&]() {
      auto rect_size = rand_size();
      auto before    = hrclock_t::now();
      auto rect      = s_packer.pack(rect_size + s_padding);
      auto after     = hrclock_t::now();
      auto time      = std::chrono::duration_cast<dur_t>(after - before).count();
      s_total_add_time += time;
      s_add_timings.emplace_back(time);
      if (rect) {
        s_surface += float(rect_size.x) * float(rect_size.y);
        s_packed.emplace_back(make_bbox_with_origin_size(u16vec2_px{*rect}, rect_size - s_padding));
      }
      return rect.has_value();
    };
    auto remove_rand = [&]() {
      auto index     = rand_int(s_packed.size());
      auto rect_size = bbox_size(s_packed[index]);
      auto before    = hrclock_t::now();
      s_packer.offer(s_packed[index]);
      auto after = hrclock_t::now();
      auto time  = std::chrono::duration_cast<dur_t>(after - before).count();
      s_total_rem_time += time;
      s_rem_timings.emplace_back(time);
      s_surface -= rect_size.x * rect_size.y;
      s_packed.erase(s_packed.begin() + index);
    };

    if (ImGui::Button("Reset")) {
      reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
      add_one();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add 1000")) {
      for (int i = 0; i < 1000; i++)
        add_one();
    }
    ImGui::SameLine();
    if (ImGui::Button("Rem")) {
      remove_rand();
    }
    ImGui::SameLine();
    if (ImGui::Button("Fill")) {
      reset();
      while (add_one())
        ;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stress")) {
      while (!s_packed.empty())
        remove_rand();
      while (add_one())
        ;
    }
    ImGui::SameLine();
    ImGui::InputScalarN("Padding", ImGuiDataType_U16, &s_padding.x, 2, nullptr, nullptr, "%d", 0);
    ImGui::Text("Packed %zu/%zu (with %zu rem), %f%% surface, %fms total add (%fms/op), %fms total rem (%fms/op)",
                s_packed.size(), s_add_timings.size(), s_rem_timings.size(),
                s_surface / float(_bounds.x) * float(_bounds.y) * 100, s_total_add_time,
                s_total_add_time / s_add_timings.size(), s_total_rem_time, s_total_rem_time / s_rem_timings.size());
    ImGui::PlotHistogram("Add", s_add_timings.data(), s_add_timings.size(), 0, nullptr, FLT_MAX, FLT_MAX,
                         ImVec2(0, 200));
    ImGui::PlotHistogram("Rem", s_rem_timings.data(), s_rem_timings.size(), 0, nullptr, FLT_MAX, FLT_MAX,
                         ImVec2(0, 200));

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2      p         = ImGui::GetCursorScreenPos();

    draw_list->AddRect(ImVec2(p.x, p.y), ImVec2(p.x + (float)_bounds.x, p.y + (float)_bounds.y),
                       IM_COL32(255, 0, 0, 255));
    for (auto &rect : s_packed) {
      draw_list->AddRectFilled(ImVec2(p.x + rect.x, p.y + rect.y), ImVec2(p.x + rect.z, p.y + rect.w),
                               IM_COL32(255, 255, 0, 127));
      draw_list->AddRect(ImVec2(p.x + rect.x, p.y + rect.y), ImVec2(p.x + rect.z, p.y + rect.w),
                         IM_COL32(255, 255, 0, 255));
    }
    debug(s_packer);

    ImGui::Dummy(ImVec2(float(_bounds.x), float(_bounds.y)));
  }
  ImGui::End();
}

int main(int /*unused*/, char ** /*unused*/) {
  display       dsp("hut binpack playground");
  shared_buffer buf = std::make_shared<buffer>(dsp);

  window win(dsp, buf);
  win.clear_color({0, 0, 0, 1});
  win.title(u8"hut binpack playground");

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!imgui::init(&dsp, &win, buf, true))
    return EXIT_FAILURE;
  install_test_events(dsp, win);

  win.on_draw_.connect([&](VkCommandBuffer _buffer) {
    imgui::new_frame();
    ImGui::NewFrame();

    packer<binpack::adaptor1d_dummy2d<u16, binpack::linear1d<u16>>>("linear1d", {32 * 1024, 100}, {5, 100});
    packer<binpack::shelve<u16, binpack::shelve_separator_align<u16, 16>>>("shelves<align<16>>");
    packer<binpack::shelve<u16, binpack::shelve_separator_pow<u16, 16>>>("shelves<pow<16>>");

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
