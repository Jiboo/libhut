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

#include "hut_imgui.hpp"

#include "tst_png.hpp"
#include "tst_events.hpp"

using namespace hut;

int main(int, char**) {
  display d("hut clipboard playground");

  window w(d);
  w.title("hut clipboard playground");
  w.clear_color({0, 0, 0, 1});

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!ImGui_ImplHut_Init(&d, &w, true))
    return EXIT_FAILURE;
  install_test_events(d, w);

  char text_input[4096];
  auto formats = clipboard_formats{} | FTEXT_PLAIN | FTEXT_HTML;

  w.on_draw.connect([&](VkCommandBuffer _buffer) {
    ImGui_ImplHut_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Text tests")) {
      ImGui::InputTextMultiline("", text_input, sizeof(text_input));
      ImGui_HutFlag("allowed formats", &formats, format_mime_type);

      if (ImGui::Button("Copy")) {
        std::cout << "Offering to clipboard" << std::endl;
        w.clipboard_offer(formats, [&text_input](clipboard_format _mime, clipboard_sender &_sender) {
          auto size = strlen(text_input);
          auto wrote = _sender.write({(u8*)text_input, size});
          std::cout << "Wrote " << wrote << "/" << size << " bytes to clipboard in " << _mime << std::endl;
        });
      }
      if (ImGui::Button("Paste")) {
        std::cout << "Requesting from clipboard" << std::endl;
        w.clipboard_receive(formats, [&text_input](clipboard_format _mime, clipboard_receiver &_receiver) {
          auto read = _receiver.read({(u8*)text_input, sizeof(text_input) - 1});
          text_input[read] = 0;
          std::cout << "Read " << read << " bytes from clipboard in " << _mime << std::endl;
          hexdump(text_input, read);
          u8 sink[1024];
          while (_receiver.read(sink));
        });
      }
    }
    ImGui::End();

    if (ImGui::Begin("Image tests")) {
      if (ImGui::Button("Copy")) {
        std::cout << "Offering to clipboard" << std::endl;
        w.clipboard_offer(clipboard_formats{} | FIMAGE_PNG, [](clipboard_format _mime, clipboard_sender &_sender) {
          auto wrote = _sender.write(tst_png::tex1_png);
          std::cout << "Wrote " << wrote << " bytes to clipboard in " << _mime << std::endl;
        });
      }
      if (ImGui::Button("Paste")) {
        std::cout << "Requesting from clipboard" << std::endl;
        w.clipboard_receive(clipboard_formats{} | FIMAGE_PNG, [](clipboard_format _mime, clipboard_receiver &_receiver) {
          std::ofstream ofs("clipboard.png", std::ios::binary);
          assert(ofs.is_open());
          u8 buffer[2048];
          while (auto read = _receiver.read(buffer)) {
            hexdump(buffer, read);
            ofs.write((char*)buffer, read);
          }
          std::cout << "Read " << ofs.tellp() << " bytes from clipboard in " << _mime << " saved to clipboard.png" << std::endl;
        });
      }
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