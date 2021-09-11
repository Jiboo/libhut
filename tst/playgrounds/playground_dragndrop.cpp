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

#include "hut/utils/string.hpp"

#include "hut/display.hpp"
#include "hut/window.hpp"

#include "hut/imgui/imgui.hpp"

#include "tst_events.hpp"

using namespace hut;

static constexpr const auto BUFF_SIZE = 4096;

struct drop_target {
  std::unique_ptr<u8[]>                                         buff_              = std::make_unique<u8[]>(BUFF_SIZE);
  clipboard_formats                                             allowed_formats_   = clipboard_formats{FTEXT_PLAIN};
  dragndrop_actions                                             allowed_actions_   = dragndrop_actions{DNDCOPY};
  dragndrop_action                                              preferred_action_  = DNDCOPY;
  std::array<clipboard_format, CLIPBOARD_FORMAT_LAST_VALUE + 1> preferred_formats_ = {
      FIMAGE_PNG, FIMAGE_JPEG, FIMAGE_BMP, FTEXT_HTML, FTEXT_URI_LIST, FTEXT_PLAIN};
  vec4 bbox_ = {0, 0, 0, 0};
};

struct my_drop_target_interface : drop_target_interface {
  std::vector<drop_target> &targets_;
  drop_target              *target_ = nullptr;
  clipboard_formats         current_formats_;
  clipboard_format          preferred_format_         = FTEXT_PLAIN;
  move_result               last_printed_move_result_ = {dragndrop_actions{DNDNONE}, DNDNONE, FTEXT_PLAIN};

  explicit my_drop_target_interface(std::vector<drop_target> &_targets)
      : targets_(_targets) {}

  void on_enter(dragndrop_actions _actions, clipboard_formats _formats) override {
    std::cout << "my_drop_target_interface::on_enter " << _actions << ", " << _formats << std::endl;
    current_formats_ = _formats;
    target_          = nullptr;
  }

  move_result on_move(vec2 _pos) override {
    //std::cout << "my_drop_target_interface::on_move " << _pos << std::endl;
    constexpr move_result default_result = {dragndrop_actions{DNDNONE}, DNDNONE, FTEXT_PLAIN};
    move_result           result         = default_result;

    for (auto &target : targets_) {
      if (bbox_contains(target.bbox_, _pos) && (target.allowed_formats_ & current_formats_)) {
        if (target_ != &target) {
          for (auto f : target.preferred_formats_) {
            if (target.allowed_formats_.query(f) && current_formats_.query(f)) {
              preferred_format_ = f;
              target_           = &target;
              break;
            }
          }
        }
        result = {target.allowed_actions_, target.preferred_action_, preferred_format_};
        break;
      }
    }

    if (result == default_result) {
      target_ = nullptr;
    }

    if (result != last_printed_move_result_) {
      std::cout << "my_drop_target_interface::on_move returning " << result.possible_actions_ << ", "
                << result.preferred_action_ << ", " << result.preferred_format_ << std::endl;
      last_printed_move_result_ = result;
    }
    return result;
  }

  void on_drop(dragndrop_action _action, clipboard_receiver &_receiver) override {
    assert(target_);
    auto read = _receiver.read(std::span(target_->buff_.get(), BUFF_SIZE));
    hexdump(target_->buff_.get(), read);
    target_->buff_.get()[read] = 0;
    std::cout << "my_drop_target_interface::on_drop " << _action << ", read " << read << " bytes from dragndrop" << std::endl;
    u8 sink[1024];
    while (_receiver.read(sink))
      ;
  }
};

int main(int, char **) {
  display dsp("hut dragndrop playground");
  window  win(dsp);
  win.title(u8"hut dragndrop playground");
  win.clear_color({0, 0, 0, 1});

  auto last_item_pos = [](vec4 *_dst) {
    auto min = ImGui::GetItemRectMin();
    auto max = ImGui::GetItemRectMax();
    *_dst    = {min.x, min.y, max.x, max.y};
  };

  char source_data[BUFF_SIZE] = "Hello world from libhut";
  auto source_formats         = clipboard_formats{FTEXT_PLAIN};
  auto source_actions         = dragndrop_actions{DNDCOPY};
  vec4 source_bbox{0, 0, 0, 0};

  std::vector<drop_target> targets;
  targets.reserve(16);
  targets.emplace_back();

  win.dragndrop_target(std::make_shared<my_drop_target_interface>(targets));
  win.on_mouse.connect([&](u8 _button, mouse_event_type _type, vec2 _coords) {
    if (_type == MDOWN && _button == 1 && bbox_contains(source_bbox, _coords)) {
      std::cout << "Starting drag with " << source_actions << ", " << source_formats << std::endl;
      win.dragndrop_start(source_actions, source_formats, [&](dragndrop_action _action, clipboard_format _mime, clipboard_sender &_sender) {
        auto size  = strlen(source_data);
        auto wrote = _sender.write({(u8 *)source_data, size});
        std::cout << "Wrote " << wrote << "/" << size << " bytes to drag target in " << _mime << std::endl;
      });
      return true;
    }
    return false;
  });

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!ImGui_ImplHut_Init(&dsp, &win, true))
    return EXIT_FAILURE;
  install_test_events(dsp, win);

  win.on_draw.connect([&](VkCommandBuffer _buffer) {
    ImGui_ImplHut_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Drag source")) {
      ImVec2 size{ImGui::GetContentRegionAvailWidth(), 100};
      ImGui::Button("Drag me", size);
      last_item_pos(&source_bbox);

      ImGui::InputTextMultiline("Source data", source_data, sizeof(source_data));

      ImGui_HutFlag("formats", &source_formats, format_mime_type);
      ImGui_HutFlag("actions", &source_actions, action_name);

      if (ImGui::Button("Add drop target")) {
        targets.emplace_back();
      }
    }
    ImGui::End();

    for (auto &target : targets) {
      constexpr auto buff_size = sizeof("Target ") + sizeof(void *) * 2;
      char           title_buff[buff_size];
      snprintf(title_buff, buff_size, "Target %p", &target);
      if (ImGui::Begin(title_buff)) {
        ImVec2 size{ImGui::GetContentRegionAvailWidth(), 100};
        ImGui::Button("Drop here", size);
        last_item_pos(&target.bbox_);

        ImGui::LabelText("Dropped data", "%s", target.buff_.get());

        ImGui_HutFlag("allowed formats", &target.allowed_formats_, format_mime_type);
        ImGui_HutFlag("allowed actions", &target.allowed_actions_, action_name);
        ImGui_HutFlagSelect<dragndrop_actions>("pref. action", &target.preferred_action_, action_name);
        ImGui_HutFlagReorder<clipboard_formats>("pref. format", target.preferred_formats_.data(), format_mime_type);
      }
      ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplHut_RenderDrawData(_buffer, ImGui::GetDrawData());
    return false;
  });

  dsp.dispatch();

  ImGui_ImplHut_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
