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

#pragma once

#include <imgui.h>

#include "hut/utils/math.hpp"

#include "hut/display.hpp"
#include "hut/window.hpp"

namespace hut::imgui {

IMGUI_IMPL_API bool init(display *_d, window *_w, bool _install_callbacks);
IMGUI_IMPL_API void shutdown();
IMGUI_IMPL_API void new_frame();
IMGUI_IMPL_API void render(VkCommandBuffer _buffer, ImDrawData *_draw_data);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API bool        create_device_objects();
IMGUI_IMPL_API void        invalidate_device_objects();
IMGUI_IMPL_API ImTextureID add_image(const shared_image &_image, const shared_sampler &_sampler);
IMGUI_IMPL_API void        rem_image(ImTextureID _id);

// Callbacks (installed by default if you enable 'install_callbacks' during initialization)
// You can also handle inputs yourself and use those as a reference.
IMGUI_IMPL_API bool handle_resize(u16vec2 _size, u32 _scale);
IMGUI_IMPL_API bool handle_mouse(u8 _button, mouse_event_type _type, vec2 _pos);
IMGUI_IMPL_API bool handle_key(keycode _kcode, keysym _ksym, bool _down);
IMGUI_IMPL_API bool handle_char(char32_t _utf32_char);
IMGUI_IMPL_API bool handle_focus(bool _focused);

template<typename TEnum, TEnum TEnd, typename TUnderlying = u32>
bool flagged_multi(const char *_label, flagged<TEnum, TEnd, TUnderlying> *_flags, const char *(*_name_cb)(TEnum)) {
  bool changed = false;
  ImGui::Text("%s: ", _label);
  for (TUnderlying i = 0; i <= static_cast<TUnderlying>(TEnd); i++) {
    auto e      = static_cast<TEnum>(i);
    bool status = _flags->query(e);
    ImGui::SameLine();
    changed |= ImGui::Checkbox(_name_cb(e), &status);
    if (status)
      _flags->set(e);
    else
      _flags->reset(e);
  }
  return changed;
}

template<typename TFlagged>
bool flagged_single(const char *_label, typename TFlagged::enum_type *_value,
                    const char *(*_name_cb)(typename TFlagged::enum_type)) {
  bool changed = false;
  int  val     = *_value;
  ImGui::Text("%s: ", _label);
  for (typename TFlagged::underlying_type i = 0; i <= TFlagged::ENUM_END; i++) {
    ImGui::SameLine();
    changed |= ImGui::RadioButton(_name_cb(static_cast<typename TFlagged::enum_type>(i)), &val, i);
  }
  *_value = static_cast<typename TFlagged::enum_type>(val);
  return changed;
}

template<typename TFlagged>
bool flagged_reorder(const char *_label, typename TFlagged::enum_type *_values,
                     const char *(*_name_cb)(typename TFlagged::enum_type)) {
  ImGui::Text("%s: ", _label);
  for (typename TFlagged::underlying_type i = 0; i <= TFlagged::ENUM_END; i++) {
    ImGui::Selectable(_name_cb(_values[i]));
    if (ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
      i64 next = i + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
      if (next >= 0 && next <= i64(TFlagged::ENUM_END)) {
        std::swap(_values[i], _values[next]);
        ImGui::ResetMouseDragDelta();
      }
    }
  }
  return false;
}

}  // namespace hut::imgui
