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
#include <cstring>

#include <windows.h>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

window::window(display &_display) : display_(_display), size_(800, 600) {
  window_ = CreateWindowExW(WS_EX_APPWINDOW, HUT_WIN32_CLASSNAME, L"hut", WS_POPUP,
    0, 0, 800, 600, NULL, NULL, display_.hinstance_, &display_);
  if (!window_) {
    throw std::runtime_error("couldn't create window");
  }

  VkWin32SurfaceCreateInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  info.hinstance = display_.hinstance_;
  info.hwnd = window_;

  auto func = display_.get_proc<PFN_vkCreateWin32SurfaceKHR>("vkCreateWin32SurfaceKHR");
  VkResult vkr;
  if ((vkr = func(display_.instance_, &info, nullptr, &surface_)) != VK_SUCCESS)
    throw std::runtime_error(sstream("couldn't create dummy surface, code: ") << vkr);

  _display.windows_.emplace(window_, this);

  init_vulkan_surface();

  ShowWindow(window_, SW_SHOWNA);
  UpdateWindow(window_);
}

window::~window() {
  close();
}

void window::close() {
  if (window_ != nullptr) {
    display_.windows_.erase(window_);

    destroy_vulkan();
    DestroyWindow(window_);

    display_.post_empty_event();
  }
}

void window::pause() {
  ShowWindow(window_, SW_MINIMIZE);
}

void window::maximize(bool _set) {
  ShowWindow(window_, _set ? SW_MAXIMIZE : SW_RESTORE);
}

void window::title(const std::string &_title) {
  auto titlew = to_wstring(_title);
  SetWindowTextW(window_, titlew.c_str());
}

void window::invalidate(const uvec4 &_coords, bool _redraw) {
  if (_redraw) {
    for (size_t i = 0; i < dirty_.size(); i++)
      dirty_[i] = true;
  }
  RECT rect;
  rect.left = _coords.x;
  rect.top = _coords.y;
  rect.right = _coords.z;
  rect.bottom = _coords.w;
  RedrawWindow(window_, &rect, NULL, RDW_INTERNALPAINT);
}
