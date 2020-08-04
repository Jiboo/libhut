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

#include <iostream>

#include <windows.h>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

window::window(display &_display) : display_(_display), size_(800, 600) {
  window_ = CreateWindowExA(WS_EX_APPWINDOW, HUT_WIN32_CLASSNAME, "hut", WS_POPUP,
    0, 0, 800, 600, nullptr, nullptr, display_.hinstance_, &display_);
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
  SetWindowTextA(window_, _title.c_str());
}

void window::invalidate(const uvec4 &_coords, bool _redraw) {
  if (_redraw) {
    for (auto &d : dirty_)
      d = true;
  }
  RECT rect;
  rect.left = _coords.x;
  rect.top = _coords.y;
  rect.right = _coords.z;
  rect.bottom = _coords.w;
  RedrawWindow(window_, &rect, nullptr, RDW_INTERNALPAINT);
}

#define AFX_IDC_CONTEXTHELP             30977       // context sensitive help
#define AFX_IDC_MAGNIFY                 30978       // print preview zoom
#define AFX_IDC_SMALLARROWS             30979       // splitter
#define AFX_IDC_HSPLITBAR               30980       // splitter
#define AFX_IDC_VSPLITBAR               30981       // splitter
#define AFX_IDC_NODROPCRSR              30982       // No Drop Cursor
#define AFX_IDC_TRACKNWSE               30983       // tracker
#define AFX_IDC_TRACKNESW               30984       // tracker
#define AFX_IDC_TRACKNS                 30985       // tracker
#define AFX_IDC_TRACKWE                 30986       // tracker
#define AFX_IDC_TRACK4WAY               30987       // tracker
#define AFX_IDC_MOVE4WAY                30988       // resize bar (server only)

HCURSOR load_cursor(cursor_type _c) {
  switch (_c) {
    case CNONE:
      return nullptr;
    case CDEFAULT:
      return LoadCursorA(nullptr, IDC_ARROW);
    case CCONTEXT_MENU:
      break;
    case CHELP:
      return LoadCursorA(nullptr, IDC_HELP);
    case CPOINTER:
      return LoadCursorA(nullptr, IDC_HAND);
    case CPROGRESS:
      return LoadCursorA(nullptr, IDC_APPSTARTING);
    case CWAIT:
      return LoadCursorA(nullptr, IDC_WAIT);
    case CCELL:
      break;
    case CCROSSHAIR:
      return LoadCursorA(nullptr, IDC_CROSS);
    case CTEXT:
      return LoadCursorA(nullptr, IDC_IBEAM);
    case CALIAS:
      break;
    case CCOPY:
      break;
    case CMOVE:
      return LoadCursorA(nullptr, IDC_SIZEALL);
    case CNO_DROP:
      return LoadCursorA(nullptr, IDC_NO);
    case CNOT_ALLOWED:
      return LoadCursorA(nullptr, IDC_NO);
    case CGRAB:
      return LoadCursorA(nullptr, IDC_SIZEALL);
    case CGRABBING:
      return LoadCursorA(nullptr, IDC_SIZEALL);
    case CSCROLL_ALL:
      return LoadCursorA(nullptr, IDC_SIZEALL);
    case CRESIZE_COL:
      return LoadCursorA(nullptr, IDC_SIZEWE);
    case CRESIZE_ROW:
      return LoadCursorA(nullptr, IDC_SIZENS);
    case CRESIZE_N:
      return LoadCursorA(nullptr, IDC_SIZENS);
    case CRESIZE_E:
      return LoadCursorA(nullptr, IDC_SIZEWE);
    case CRESIZE_S:
      return LoadCursorA(nullptr, IDC_SIZENS);
    case CRESIZE_W:
      return LoadCursorA(nullptr, IDC_SIZEWE);
    case CRESIZE_NE:
      return LoadCursorA(nullptr, IDC_SIZENESW);
    case CRESIZE_NW:
      return LoadCursorA(nullptr, IDC_SIZENWSE);
    case CRESIZE_SE:
      return LoadCursorA(nullptr, IDC_SIZENWSE);
    case CRESIZE_SW:
      return LoadCursorA(nullptr, IDC_SIZENESW);
    case CRESIZE_EW:
      return LoadCursorA(nullptr, IDC_SIZEWE);
    case CRESIZE_NS:
      return LoadCursorA(nullptr, IDC_SIZENS);
    case CRESIZE_NESW:
      return LoadCursorA(nullptr, IDC_SIZENESW);
    case CRESIZE_NWSE:
      return LoadCursorA(nullptr, IDC_SIZENWSE);
    case CZOOM_IN:
      break;
    case CZOOM_OUT:
      break;
  }
  return LoadCursorA(nullptr, IDC_ARROW);
}

void window::set_cursor(cursor_type _c) {
  SetCursor(load_cursor(_c));
  current_cursor_type_ = _c;
}
