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

#include "hut/utils/chrono.hpp"

#include "hut/display.hpp"
#include "hut/window.hpp"

#include "hut/imgui/imgui.hpp"

#include "tst_events.hpp"

using namespace hut;

int main(int /*unused*/, char ** /*unused*/) {
  display dsp("hut demo");

  window win(dsp);
  win.clear_color({0, 0, 0, 1});
  win.title(u8"hut imgui demo");

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!imgui::init(&dsp, &win, true))
    return EXIT_FAILURE;
  install_test_events(dsp, win);

  vec4 clear_color{0, 0, 0, 1};

  bool show_demo_window    = true;
  bool show_another_window = true;

  win.on_draw_.connect([&](VkCommandBuffer _buffer) {
    imgui::new_frame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
      ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    static float s_f       = 0.0f;
    static int   s_counter = 0;

    if (ImGui::Begin("Hello, world!")) {                  // Create a window called "Hello, world!" and append into it.
      ImGui::Text("This is some useful text.");           // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_window);  // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      ImGui::SliderFloat("float", &s_f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit4("clear color", (float *)&clear_color);  // Edit 3 floats representing a color

      if (ImGui::Button("Button"))  // Buttons return true when clicked (most widgets return true when edited/activated)
        s_counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", s_counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                  ImGui::GetIO().Framerate);
    }
    ImGui::End();

    // 3. Show another simple window.
    if (show_another_window) {
      if (ImGui::Begin(
              "Another Window",
              &show_another_window)) {  // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
          show_another_window = false;
      }
      ImGui::End();
    }

    ImGui::Render();
    imgui::render(_buffer, ImGui::GetDrawData());

    return false;
  });
  win.on_frame_.connect([&](display::duration _dt) {
    dsp.post([&](auto) { win.invalidate(true); });
    return false;
  });

  win.on_key_.connect([](keycode, keysym _sym, bool _down) {
#ifdef HUT_ENABLE_PROFILING
    if (_sym == KSYM_P && _down)
      profiling::threads_data::request_dump();
#endif  // HUT_ENABLE_PROFILING
    if (_sym == KSYM_F && _down)
      std::this_thread::sleep_for(100ms);
    return false;
  });

  int error_code = dsp.dispatch();

  imgui::shutdown();
  ImGui::DestroyContext();

  return error_code;
}
