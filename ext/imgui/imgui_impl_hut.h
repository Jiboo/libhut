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

IMGUI_IMPL_API bool     ImGui_ImplHut_Init(hut::display *_d, hut::window* _w, bool _install_callbacks);
IMGUI_IMPL_API void     ImGui_ImplHut_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplHut_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplHut_RenderDrawData(VkCommandBuffer _buffer, ImDrawData* _draw_data);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API bool     ImGui_ImplHut_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplHut_InvalidateDeviceObjects();

// Callbacks (installed by default if you enable 'install_callbacks' during initialization)
// You can also handle inputs yourself and use those as a reference.
IMGUI_IMPL_API bool     ImGui_ImplHut_HandleOnResize(hut::uvec2 _size);
IMGUI_IMPL_API bool     ImGui_ImplHut_HandleOnMouse(uint8_t _button, hut::mouse_event_type _type, hut::vec2 _pos);
IMGUI_IMPL_API bool     ImGui_ImplHut_HandleOnKey(hut::keycode _kcode, hut::keysym _ksym, bool _down);
IMGUI_IMPL_API bool     ImGui_ImplHut_HandleOnKMods(hut::modifiers _mods);
IMGUI_IMPL_API bool     ImGui_ImplHut_HandleOnChar(char32_t _utf32_char);
