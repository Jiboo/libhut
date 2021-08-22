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

#include <cstdint>

#include <glm/glm.hpp>

#include "imgui.h"

#include "hut/window.hpp"
#include "hut/display.hpp"

IMGUI_IMPL_API bool ImGui_ImplHut_Init(hut::display *_d, hut::window* _w, bool _install_callbacks);
IMGUI_IMPL_API void ImGui_ImplHut_Shutdown();
IMGUI_IMPL_API void ImGui_ImplHut_NewFrame();
IMGUI_IMPL_API void ImGui_ImplHut_RenderDrawData(VkCommandBuffer _buffer, ImDrawData* _draw_data);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API bool ImGui_ImplHut_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplHut_InvalidateDeviceObjects();
IMGUI_IMPL_API ImTextureID ImGui_ImplHut_AddImage(hut::shared_image _image, hut::shared_sampler _sampler);
IMGUI_IMPL_API void ImGui_ImplHut_RemImage(ImTextureID);

// Callbacks (installed by default if you enable 'install_callbacks' during initialization)
// You can also handle inputs yourself and use those as a reference.
IMGUI_IMPL_API bool ImGui_ImplHut_HandleOnResize(hut::uvec2 _size);
IMGUI_IMPL_API bool ImGui_ImplHut_HandleOnMouse(uint8_t _button, hut::mouse_event_type _type, hut::vec2 _pos);
IMGUI_IMPL_API bool ImGui_ImplHut_HandleOnKey(hut::keycode _kcode, hut::keysym _ksym, bool _down);
IMGUI_IMPL_API bool ImGui_ImplHut_HandleOnKMods(hut::modifiers _mods);
IMGUI_IMPL_API bool ImGui_ImplHut_HandleOnChar(char32_t _utf32_char);

template<typename TEnum, TEnum TEnd, typename TUnderlying = uint32_t>
bool ImGui_HutFlag(const char *_label, hut::flagged<TEnum, TEnd, TUnderlying> *_flags,
                        const char *(*_name_cb)(TEnum)) {
  bool changed = false;
  ImGui::Text("%s: ", _label);
  for (TUnderlying i = 0; i <= static_cast<TUnderlying>(TEnd); i++) {
    auto e = static_cast<TEnum>(i);
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
bool ImGui_HutFlagSelect(const char *_label, typename TFlagged::enum_type *_value,
                         const char *(*_name_cb)(typename TFlagged::enum_type)) {
  bool changed = false;
  int val = *_value;
  ImGui::Text("%s: ", _label);
  for (typename TFlagged::underlying_type i = 0; i <= TFlagged::enum_end; i++) {
    ImGui::SameLine();
    changed |= ImGui::RadioButton(_name_cb(static_cast<typename TFlagged::enum_type>(i)), &val, i);
  }
  *_value = static_cast<typename TFlagged::enum_type>(val);
  return changed;
}

template<typename TFlagged>
bool ImGui_HutFlagReorder(const char *_label, typename TFlagged::enum_type *_values,
                         const char *(*_name_cb)(typename TFlagged::enum_type)) {
  ImGui::Text("%s: ", _label);
  for (typename TFlagged::underlying_type i = 0; i <= TFlagged::enum_end; i++) {
    ImGui::Selectable(_name_cb(_values[i]));
    if (ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
      int next = i + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
      if (next >= 0 && next <= TFlagged::enum_end) {
        std::swap(_values[i], _values[next]);
        ImGui::ResetMouseDragDelta();
        //return true;
      }
    }
  }
  return false;
}
