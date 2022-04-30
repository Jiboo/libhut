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

#define HUT_TEXT_SAMPLE_ENABLE_IMGUI

#include <random>

#include "hut/utils/color.hpp"
#include "hut/utils/string.hpp"
#include "hut/utils/utf.hpp"

#include "hut/atlas.hpp"
#include "hut/display.hpp"
#include "hut/pipeline.hpp"
#include "hut/window.hpp"

#ifdef HUT_TEXT_SAMPLE_ENABLE_IMGUI
#  include "hut/imgui/imgui.hpp"
#endif

#include "hut/text/renderer.hpp"

#include "tst_events.hpp"
#include "tst_woff2.hpp"

using namespace hut;

int main(int /*unused*/, char ** /*unused*/) {
  display       dsp("hut text playground");
  shared_buffer buf = std::make_shared<buffer>(dsp);

  window win(dsp, buf);
  win.clear_color({0, 0, 0, 1});
  win.title(u8"hut text playground");

  auto ubo = buf->allocate<common_ubo>(1, dsp.ubo_align());
  ubo->set(common_ubo{win.size()});
  auto fontatlas
      = std::make_shared<atlas>(dsp, buf, image_params{.size_ = dsp.max_tex_size(), .format_ = VK_FORMAT_R8_UNORM});
  auto           samp      = std::make_shared<sampler>(dsp);
  constexpr auto FONT_SIZE = 16_px;
  auto           font      = std::make_shared<text::font>(tst_woff2::Roboto_Regular_woff2, FONT_SIZE);

#ifdef HUT_TEXT_SAMPLE_ENABLE_IMGUI
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!imgui::init(&dsp, &win, buf, true))
    return EXIT_FAILURE;
#endif
  install_test_events(dsp, win, ubo);

  text::renderer_params params;
  /*params.initial_draw_store_size_ = 10;
  params.initial_mesh_store_size_ = 8 * 10;*/
  text::renderer r(win, buf, font, ubo, fontatlas, samp, params);

  char8_t text_input[1024 * 1024];

  std::mt19937 gen;
  auto         gen_norm = [&]() {
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
  };
  auto gen_uint = [&](uint _min, uint _max) {
    std::uniform_int_distribution<> dis(_min, _max);
    return dis(gen);
  };
  auto gen_vowel = [&]() {
    constexpr char8_t               VOWELS[] = {'a', 'e', 'y', 'u', 'i', 'o'};
    std::uniform_int_distribution<> dis(0, sizeof(VOWELS) - 1);
    return VOWELS[dis(gen)];
  };
  auto gen_consonant = [&]() {
    constexpr char8_t CONSONANTS[]
        = {'z', 'r', 't', 'p', 'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'w', 'x', 'c', 'v', 'b', 'n'};
    std::uniform_int_distribution<> dis(0, sizeof(CONSONANTS) - 1);
    return CONSONANTS[dis(gen)];
  };
  auto gen_word = [&](char8_t *_dst, bool _start) {
    uint word_size = gen_uint(2, 10);
    for (uint i = 0; i < word_size; i++) {
      f32  current = gen_norm();
      char c       = current > 0.66 ? gen_consonant() : gen_vowel();
      *_dst++      = (i == 0 && _start) ? toupper(c) : c;
    }
    return _dst;
  };
  auto gen_phrase = [&](char8_t *_dst) {
    uint phrase_size = gen_uint(4, 20);
    for (uint i = 0; i < phrase_size; i++) {
      _dst = gen_word(_dst, i == 0);
      if (i != (phrase_size - 1)) {
        f32 comma = gen_norm();
        if (comma > 0.9)
          *_dst++ = ',';
        *_dst++ = ' ';
      } else {
        *_dst++ = '.';
      }
    }
    return _dst;
  };
  auto gen_paragraph = [&](char8_t *_dst) {
    uint paragraph_size = gen_uint(4, 8);
    for (uint i = 0; i < paragraph_size; i++) {
      _dst    = gen_phrase(_dst);
      *_dst++ = (i == paragraph_size - 1) ? '\n' : ' ';
    }
    return _dst;
  };
  auto gen_text = [&](char8_t *_dst, uint _min_paragraph, uint _max_paragraph) {
    uint text_size = gen_uint(_min_paragraph, _max_paragraph);
    for (uint i = 0; i < text_size; i++) {
      _dst = gen_paragraph(_dst);
    }
    *_dst++ = 0;
    return _dst;
  };
  gen_text(text_input, 10, 30);

  std::vector<std::u8string_view> words{16000};

  auto dosplit = [&words, &text_input]() {
    words.clear();
    auto add_word = [&words](std::u8string_view _view) { words.emplace_back(_view); };
    split(std::u8string_view{text_input}, u8" \n\r\t"sv, add_word);
  };

  using clock       = display::clock;
  auto before_split = clock::now();
  dosplit();
  auto after_split = clock::now();

  auto before_release = clock::now();
  auto after_release  = clock::now();
  auto before_alloc   = clock::now();
  auto paragraph      = r.allocate(words);
  auto after_alloc    = clock::now();
  auto before_update  = clock::now();
  auto after_update   = clock::now();

  u32  current_scale          = win.scale();
  bool release_before_realloc = false;

  auto relayout = [&](u16vec2_px _size, u32 _scale) {
    before_update = clock::now();
    if (_scale != current_scale) {
      current_scale = _scale;
      font->reset_to_size(FONT_SIZE * _scale);
      paragraph = r.allocate(words);
    }
    const auto win_width   = _size.x * _scale;
    u32_px     line_height = FONT_SIZE * _scale;
    u32_px     space_width = line_height / 4;
    uvec2_px   offset      = {0, line_height};
    auto       staging     = paragraph.instances().update();
    for (uint i = 0; i < paragraph.size(); i++) {
      const auto bbox       = paragraph.bbox(i);
      const auto line_break = (offset.x + bbox.z) > win_width;
      if (line_break) {
        offset.x = 0;
        offset.y += line_height;
      }
      staging[i] = {offset, colors::WHITE};
      offset.x += bbox.z + space_width;
    }
    after_update = clock::now();
    return false;
  };
  relayout(win.size(), win.scale());

  win.on_draw_.connect([&](VkCommandBuffer _buffer) {
    r.draw(_buffer);

#ifdef HUT_TEXT_SAMPLE_ENABLE_IMGUI
    imgui::new_frame();
    ImGui::NewFrame();

    if (ImGui::Begin("Hello, world!")) {
      ImGui::InputTextMultiline("##text_input", (char *)text_input, sizeof(text_input));
      if (ImGui::Button("Update")) {
        before_split = clock::now();
        dosplit();
        after_split = clock::now();

        if (release_before_realloc) {
          before_release = clock::now();
          paragraph.release();
          after_release = clock::now();
        }

        before_alloc = clock::now();
        paragraph    = r.allocate(words);
        after_alloc  = clock::now();

        before_update = clock::now();
        relayout(win.size(), win.scale());
        after_update = clock::now();
      }
      ImGui::SameLine();
      ImGui::Checkbox("Free before", &release_before_realloc);

      using msdur = std::chrono::duration<float, std::milli>;
      ImGui::Text("Words %zu, codepoints %zu, size %zu", words.size(), utf8_codepoint_count(text_input),
                  strlen((char *)text_input));
      ImGui::Text("Parent %p, Offset: %d, size: %d", paragraph.instances().parent(),
                  paragraph.instances().offset_bytes(), paragraph.instances().size_bytes());
      ImGui::Text("Split %fms", msdur(after_split - before_split).count());
      if (release_before_realloc) {
        ImGui::Text("Release %fms", msdur(after_release - before_release).count());
      }
      msdur alloc_dur = after_alloc - before_alloc;
      ImGui::Text("Alloc %fms (%fms/word)", alloc_dur.count(), alloc_dur.count() / float(words.size()));
      msdur update_dur = after_update - before_update;
      ImGui::Text("Layout %fms (%fms/word)", update_dur.count(), update_dur.count() / float(words.size()));

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                  ImGui::GetIO().Framerate);
    }
    ImGui::End();

    ImGui::Render();
    imgui::render(_buffer, ImGui::GetDrawData());
#endif

    return false;
  });

#ifdef HUT_TEXT_SAMPLE_ENABLE_IMGUI
  win.on_frame_.connect([&](display::duration _dt) {
    dsp.post([&](auto) { win.invalidate(true); });
    return false;
  });
#endif

  win.on_key_.connect([&](keycode, keysym _sym, bool _down) {
    if (_sym == KSYM_E && _down) {
      paragraph = r.allocate(words);
      relayout(win.size(), win.scale());
      win.invalidate(true);
    } else if (_sym == KSYM_R && _down) {
      paragraph.release();
      paragraph = r.allocate(words);
      relayout(win.size(), win.scale());
      win.invalidate(true);
    }
    return false;
  });

  win.on_resize_.connect(relayout);

  dsp.dispatch();

#ifdef HUT_TEXT_SAMPLE_ENABLE_IMGUI
  imgui::shutdown();
  ImGui::DestroyContext();
#endif

  return EXIT_SUCCESS;
}
