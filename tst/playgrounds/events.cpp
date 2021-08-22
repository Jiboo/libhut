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

#include "hut/display.hpp"
#include "hut/window.hpp"

#include "hut/imgui/imgui.hpp"

#include "tst_events.hpp"

using namespace hut;

int main(int, char**) {
  display d("hut events playground");

  window w(d);
  w.title("hut events playground");
  w.clear_color({0, 0, 0, 1});

  bool handle_on_key_events = false;
  bool handle_on_close_events = false;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!ImGui_ImplHut_Init(&d, &w, true))
    return EXIT_FAILURE;

  w.on_pause.connect([&]() {
    std::cout << "on_pause" << std::endl;
    return false;
  });
  w.on_resume.connect([&]() {
    std::cout << "on_resume" << std::endl;
    return false;
  });
  w.on_focus.connect([&]() {
    std::cout << "on_focus" << std::endl;
    return false;
  });
  w.on_blur.connect([&]() {
    std::cout << "on_blur" << std::endl;
    return false;
  });
  w.on_close.connect([&]() {
    std::cout << "on_close" << std::endl;
    return handle_on_close_events;
  });

  w.on_expose.connect([&](uvec4 _dirty) {
    std::cout << "on_expose " << _dirty << std::endl;
    return false;
  });
  w.on_resize.connect([&](uvec2 _size) {
    std::cout << "on_resize " << _size << std::endl;
    return false;
  });

  struct datapoint {
    std::array<float, 512> samples = {};
    float min = FLT_MAX, max = FLT_MIN, avg = 0;
    u32 count = samples.size() - 1;

    void add(float sample) {
      count++;
      if (count >= samples.size()) {
        count = 0;
        min = FLT_MAX;
        max = FLT_MIN;
        avg = 0;
      }

      samples[count] = sample;
      if (sample > max)
        max = sample;
      if (sample < min)
        min = sample;
      avg += sample;
    }

    float average() const {
      return avg / float(count);
    }
  };

  datapoint latencies;
  datapoint separations;
  u32 events_per_seconds = 0;
  auto last_event = display::clock::now();
  auto last_report = display::clock::now();
  auto events_period = display::clock::duration::max();

  w.on_time.connect([&](u32 _time) {
    auto handled = display::clock::now();

    auto clock_time = std::chrono::time_point<display::clock>(std::chrono::duration<u32, std::milli>(_time));
    auto latency = std::chrono::duration<float, std::milli>(handled - clock_time);
    auto separation = std::chrono::duration<float, std::milli>(handled - last_event);

    latencies.add(latency.count());
    separations.add(separation.count());

    last_event = handled;

    events_per_seconds++;
    events_period = handled - last_report;
    if (events_period > std::chrono::seconds(1)) {
      events_per_seconds = 0;
      last_report = handled;
    }

    return false;
  });

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
    //std::cout << "on_draw " << std::endl;
    ImGui_ImplHut_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Default overrides")) {
      ImGui::Checkbox("on_key", &handle_on_key_events);
      ImGui::Checkbox("on_close", &handle_on_close_events);
    }
    ImGui::End();

    if (ImGui::Begin("Time data")) {
      char header[128];
      snprintf(header, sizeof(header), "min/avg/max: %f/%f/%f", latencies.min, latencies.average(), latencies.max);
      ImGui::PlotHistogram("latencies", latencies.samples.data(), latencies.samples.size(), 0, header, 0, 1000/24.f, ImVec2(latencies.samples.size(), 100));

      snprintf(header, sizeof(header), "min/avg/max: %f/%f/%f", separations.min, separations.average(), separations.max);
      ImGui::PlotHistogram("separations", separations.samples.data(), separations.samples.size(), 0, header, 0, 1000/24.f, ImVec2(separations.samples.size(), 100));

      ImGui::Text("Event per second: %f", events_per_seconds / std::chrono::duration<double>(events_period).count());
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplHut_RenderDrawData(_buffer, ImGui::GetDrawData());
    return false;
  });
  w.on_frame.connect([&](display::duration _dt) {
    //std::cout << "on_frame " << _dt << std::endl;
    d.post([&](auto){
      w.invalidate(true);
    });
    return false;
  });

  w.on_touch.connect([&](u8 _finger, touch_event_type _type, vec2 _pos) {
    std::cout << "on_touch " << (int)_finger << " " << _type << " " << _pos << std::endl;
    return false;
  });
  w.on_mouse.connect([&](u8 _button, mouse_event_type _type, vec2 _pos) {
    if (_type == MMOVE)
      return false;
    std::cout << "on_mouse " << (int)_button << " " << _type << " " << _pos << std::endl;
    return false;
  });

  w.on_kmods.connect([&](modifiers _mods) {
    std::cout << "on_kmods " << _mods << std::endl;
    return false;
  });
  w.on_key.connect([&](keycode _kcode, keysym _ksym, bool _down) {
    char kcode_name[128];
    d.keycode_name(kcode_name, _kcode);
    std::cout << "on_key " << _kcode << " (" << kcode_name << "), " << _ksym << (_down ? " down" : " up") << std::endl;

    if (_ksym == KSYM_ESC && !_down)
      std::cout << "use alt+f4 to exit" << std::endl;

    return handle_on_key_events;
  });
  w.on_char.connect([&](char32_t _utf32) {
    char8_t utf8[6];
    *to_utf8(utf8, _utf32) = 0;
    std::cout << "on_char " << (char*)utf8 << std::endl;
    return false;
  });

  install_resizable_movable(w);
  install_continuous_redraw(d, w);

  d.dispatch();
  ImGui_ImplHut_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
