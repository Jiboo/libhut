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

#include "hut/utils/utf.hpp"

#include "hut/display.hpp"
#include "hut/window.hpp"

#include "hut/imgui/imgui.hpp"

#include "tst_events.hpp"

using namespace hut;

int main(int /*unused*/, char ** /*unused*/) {
  display dsp("hut events playground");

  window win(dsp);
  win.title(u8"hut events playground");
  win.clear_color({0, 0, 0, 1});

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!imgui::init(&dsp, &win, true))
    return EXIT_FAILURE;

  bool handle_on_pause_events = false;
  win.on_pause_.connect([&]() {
    std::cout << "on_pause" << std::endl;
    return handle_on_pause_events;
  });

  bool handle_on_resume_events = false;
  win.on_resume_.connect([&]() {
    std::cout << "on_resume" << std::endl;
    return handle_on_resume_events;
  });

  bool handle_on_focus_events = false;
  win.on_focus_.connect([&](bool _focused) {
    std::cout << "on_focus " << _focused << std::endl;
    return handle_on_focus_events;
  });

  bool handle_on_close_events = false;
  win.on_close_.connect([&]() {
    std::cout << "on_close" << std::endl;
    return handle_on_close_events;
  });

  bool handle_on_expose_events = false;
  win.on_expose_.connect([&](uvec4 _dirty) {
    std::cout << "on_expose " << _dirty << std::endl;
    return handle_on_expose_events;
  });

  bool handle_on_resize_events = false;
  win.on_resize_.connect([&](u16vec2_px _size, u32 _scale) {
    std::cout << "on_resize " << _size << ", " << _scale << std::endl;
    return handle_on_resize_events;
  });

  struct datapoint {
    std::array<float, 512> samples_ = {};
    float                  min_ = FLT_MAX, max_ = FLT_MIN, avg_ = 0;
    u32                    count_ = samples_.size() - 1;

    void add(float _sample) {
      count_++;
      if (count_ >= samples_.size()) {
        count_ = 0;
        min_   = FLT_MAX;
        max_   = FLT_MIN;
        avg_   = 0;
      }

      samples_[count_] = _sample;
      if (_sample > max_)
        max_ = _sample;
      if (_sample < min_)
        min_ = _sample;
      avg_ += _sample;
    }

    [[nodiscard]] float average() const { return avg_ / float(count_); }
  };

  bool handle_on_touch_events = false;
  win.on_touch_.connect([&](u8 _finger, touch_event_type _type, vec2 _pos) {
    std::cout << "on_touch " << (int)_finger << " " << _type << " " << _pos << std::endl;
    return handle_on_touch_events;
  });

  bool handle_on_mouse_events = false;
  win.on_mouse_.connect([&](u8 _button, mouse_event_type _type, vec2 _pos) {
    if (_type == MMOVE)
      return handle_on_mouse_events;
    std::cout << "on_mouse " << (int)_button << " " << _type << " " << _pos << std::endl;
    return handle_on_mouse_events;
  });

  bool handle_on_kmods_events = false;
  win.on_kmods_.connect([&](modifiers _mods) {
    std::cout << "on_kmods " << _mods << std::endl;
    return handle_on_kmods_events;
  });

  bool handle_on_key_events = false;
  win.on_key_.connect([&](keycode _kcode, keysym _ksym, bool _down) {
    char  kcode_name[128];
    char *end = dsp.keycode_name(kcode_name, _kcode);
    *end      = 0;

    std::cout << "on_key " << _kcode << " (" << kcode_name << "), " << _ksym << (_down ? " down" : " up") << std::endl;

    if (_ksym == KSYM_ESC && !_down)
      std::cout << "use alt+f4 to exit" << std::endl;

    return handle_on_key_events;
  });

  bool handle_on_char_events = false;
  win.on_char_.connect([&](char32_t _utf32) {
    char8_t utf8[6];
    *to_utf8(utf8, _utf32) = 0;
    std::cout << "on_char " << (char *)utf8 << std::endl;
    return handle_on_char_events;
  });

#if defined(HUT_ENABLE_TIME_EVENTS)
  datapoint latencies;
  datapoint separations;
  u32       events_per_seconds = 0;
  auto      last_event         = display::clock::now();
  auto      last_report        = display::clock::now();
  auto      events_period      = display::clock::duration::max();

  bool handle_on_time_events = false;
  win.on_time_.connect([&](u32 _time) {
    auto handled = display::clock::now();

    auto clock_time = std::chrono::time_point<display::clock>(std::chrono::duration<u32, std::milli>(_time));
    auto latency    = std::chrono::duration<float, std::milli>(handled - clock_time);
    auto separation = std::chrono::duration<float, std::milli>(handled - last_event);

    latencies.add(latency.count());
    separations.add(separation.count());

    last_event = handled;

    events_per_seconds++;
    events_period = handled - last_report;
    if (events_period > std::chrono::seconds(1)) {
      events_per_seconds = 0;
      last_report        = handled;
    }

    return handle_on_time_events;
  });
#endif

  bool handle_on_frame_events = false;
  win.on_frame_.connect([&](display::duration _dt) {
    //std::cout << "on_frame " << _dt << std::endl;
    dsp.post([&](auto) { win.invalidate(true); });
    return handle_on_frame_events;
  });

  bool handle_on_draw_events = false;
  win.on_draw_.connect([&](VkCommandBuffer _buffer) {
    //std::cout << "on_draw " << std::endl;
    imgui::new_frame();
    ImGui::NewFrame();

    if (ImGui::Begin("Default overrides")) {
      ImGui::Checkbox("on_pause", &handle_on_pause_events);
      ImGui::Checkbox("on_resume", &handle_on_resume_events);
      ImGui::Checkbox("on_focus", &handle_on_focus_events);
      ImGui::Separator();

      ImGui::Checkbox("on_close", &handle_on_close_events);
      ImGui::Checkbox("on_expose", &handle_on_expose_events);
      ImGui::Checkbox("on_resize", &handle_on_resize_events);
      ImGui::Separator();

      ImGui::Checkbox("on_touch", &handle_on_touch_events);
      ImGui::Checkbox("on_mouse", &handle_on_mouse_events);
      ImGui::Checkbox("on_kmods", &handle_on_kmods_events);
      ImGui::Checkbox("on_key", &handle_on_key_events);
      ImGui::Checkbox("on_char", &handle_on_char_events);
      ImGui::Separator();

#if defined(HUT_ENABLE_TIME_EVENTS)
      ImGui::Checkbox("on_time", &handle_on_time_events);
#endif
      ImGui::Checkbox("on_frame", &handle_on_frame_events);
      ImGui::Checkbox("on_draw", &handle_on_draw_events);
    }
    ImGui::End();

#if defined(HUT_ENABLE_TIME_EVENTS)
    if (ImGui::Begin("Time data")) {
      char header[128];
      snprintf(header, sizeof(header), "min/avg/max: %f/%f/%f", latencies.min_, latencies.average(), latencies.max_);
      ImGui::PlotHistogram("latencies", latencies.samples_.data(), latencies.samples_.size(), 0, header, 0, 1000 / 24.f,
                           ImVec2(latencies.samples_.size(), 100));

      snprintf(header, sizeof(header), "min/avg/max: %f/%f/%f", separations.min_, separations.average(),
               separations.max_);
      ImGui::PlotHistogram("separations", separations.samples_.data(), separations.samples_.size(), 0, header, 0,
                           1000 / 24.f, ImVec2(separations.samples_.size(), 100));

      ImGui::Text("Event per second: %f", events_per_seconds / std::chrono::duration<double>(events_period).count());
    }
    ImGui::End();
#endif

    ImGui::Render();
    imgui::render(_buffer, ImGui::GetDrawData());
    return handle_on_draw_events;
  });

  install_resizable_movable(win);

  dsp.dispatch();
  imgui::shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
