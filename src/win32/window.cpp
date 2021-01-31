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

#include <charconv>
#include <iostream>

#include <windows.h>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

window::window(display &_display, const window_params &_init_params)
  : display_(_display), size_(bbox_size(_init_params.position_)) {

  DWORD winExStyle = WS_EX_APPWINDOW;
  DWORD winStyle = WS_POPUP;
  LPCSTR className = HUT_WIN32_CLASSNAME_CSD;
  if (_init_params.flags_ & window_params::SYSTEM_DECORATIONS) {
    winExStyle = WS_EX_CLIENTEDGE;
    winStyle = WS_OVERLAPPEDWINDOW;
    className = HUT_WIN32_CLASSNAME_SSD;
  }

  window_ = CreateWindowExA(winExStyle, className, "hut", winStyle,
          _init_params.position_.x, _init_params.position_.x, size_.x, size_.y,
          nullptr, nullptr, display_.hinstance_, &display_);
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

void window::fullscreen(bool _set) {
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

void window::cursor(cursor_type _c) {
  SetCursor(load_cursor(_c));
  current_cursor_type_ = _c;
}

size_t window::clipboard_sender::write(span<uint8_t> _data) {
  buffer_.insert(buffer_.cend(), _data.begin(), _data.end());
  return _data.size_bytes();
}
size_t window::clipboard_receiver::read(span<uint8_t> _data) {
  size_t remaining = buffer_.size() - offset_;
  size_t reading = std::min(remaining, _data.size());
  memcpy(_data.data(), buffer_.data() + offset_, reading);
  offset_ += reading;
  return reading;
}

int parse_html_header(std::string_view _input, const char *_name) {
  auto offset = _input.find(_name);
  if (offset == std::string_view::npos)
    return -1;
  auto eol = _input.find('\n', offset);
  if (eol == std::string_view::npos)
    return -1;

  auto valueStart = offset + strlen(_name) + 1;
  auto substr = _input.substr(valueStart, eol - valueStart);

  int result = -1;
  std::from_chars(substr.begin(), substr.end(), result);
  return result;
}

span<uint8_t> window::parse_html_clipboard(std::string_view _input) {
  if (!_input.starts_with("Version:0.9"))
    return {};

  auto start = parse_html_header(_input, "StartFragment");
  auto end = parse_html_header(_input, "EndFragment");
  if (start <= 0 || end <= 0)
    return {};

  return span<uint8_t>((uint8_t*)_input.data() + start, (uint8_t*)_input.data() + end);
}

std::vector<uint8_t> window::format_html_clipboard(span<uint8_t> _input) {
  constexpr std::string_view header = "Version:0.9\n"
                                      "StartHTML:00000000\n"
                                      "EndHTML:00000000\n"
                                      "StartFragment:00000000\n"
                                      "EndFragment:00000000\n"
                                      "<html><body>\n"
                                      "<!--StartFragment -->\n";
  constexpr std::string_view footer = "<!--EndFragment-->\n"
                                      "</body>\n"
                                      "</html>";

  std::vector<uint8_t> result;
  result.resize(header.size() + _input.size() + footer.size() + 1);
  memcpy(result.data(), header.data(), header.size());
  memcpy(result.data() + header.size(), _input.data(), _input.size());
  memcpy(result.data() + header.size() + _input.size(), footer.data(), footer.size());
  result.back() = 0;

  constexpr auto startHtml = header.find("StartHTML:") + strlen("StartHTML:");
  constexpr auto endHTML = header.find("EndHTML:") + strlen("EndHTML:");
  constexpr auto startFrag = header.find("StartFragment:") + strlen("StartFragment:");
  constexpr auto endFrag = header.find("EndFragment:") + strlen("EndFragment:");

  constexpr auto htmlOpenTagPos = header.find("<html>");
  constexpr auto htmlCloseTagPos = footer.find("</html>") + strlen("</html>");
  constexpr auto htmlOpenFragPos = header.find("<!--StartFragment -->");
  constexpr auto htmlCloseFragPos = footer.find("<!--EndFragment-->") + strlen("<!--EndFragment-->");

  char buff[9];
  sprintf(buff, "%08u", htmlOpenTagPos);
  memcpy(result.data() + startHtml, buff, 8);

  sprintf(buff, "%08u", header.size() + _input.size() + htmlCloseTagPos);
  memcpy(result.data() + endHTML, buff, 8);

  sprintf(buff, "%08u", htmlOpenFragPos);
  memcpy(result.data() + startFrag, buff, 8);

  sprintf(buff, "%08u", header.size() + _input.size() + htmlCloseFragPos);
  memcpy(result.data() + endFrag, buff, 8);

  return result;
}

void window::clipboard_offer(clipboard_formats _supported_formats, const send_clipboard_data &_callback) {
  current_clipboard_formats_ = _supported_formats;
  current_clipboard_sender_ = _callback;

  assert(OpenClipboard(window_));
  assert(EmptyClipboard());
  for (auto format : _supported_formats) {
    switch (format) {
      case FIMAGE_PNG: [[fallthrough]]; case FIMAGE_JPEG: break; // TODO JBL: Convert to DIB
      case FIMAGE_BMP:
        SetClipboardData(CF_DIB, nullptr);
        break;
      case FTEXT_URI_LIST: [[fallthrough]];
      case FTEXT_PLAIN:
        SetClipboardData(CF_UNICODETEXT, nullptr);
        break;
      case FTEXT_HTML:
        SetClipboardData(display_.html_clipboard_format_, nullptr);
        break;
    }
  }
  assert(CloseClipboard());
}

void window::clipboard_write(clipboard_format _format, UINT _win_format) {
  window::clipboard_sender sender{};
  current_clipboard_sender_(_format, sender);

  switch(_win_format) {
    case CF_UNICODETEXT: {
      auto characterCount = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char*)sender.buffer_.data(), sender.buffer_.size(), nullptr, 0);
      if (!characterCount)
        return;
      HANDLE object = GlobalAlloc(GMEM_MOVEABLE, (characterCount + 1) * sizeof(WCHAR));
      if (!object)
        return;
      auto *buffer = (WCHAR*)GlobalLock(object);
      if (!buffer) {
        GlobalFree(object);
        return;
      }
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char*)sender.buffer_.data(), sender.buffer_.size(), buffer, characterCount);
      buffer[characterCount] = 0;
      GlobalUnlock(object);
      SetClipboardData(_win_format, object);
    } break;
    case CF_DIB: {
      HANDLE object = GlobalAlloc(GMEM_MOVEABLE, sender.buffer_.size());
      if (!object)
        return;
      auto *buffer = (uint8_t*)GlobalLock(object);
      if (!buffer) {
        GlobalFree(object);
        return;
      }
      memcpy(buffer, sender.buffer_.data(), sender.buffer_.size());
      GlobalUnlock(object);
      SetClipboardData(_win_format, object);
    } break;
  }
  if (_win_format == display_.html_clipboard_format_) {
    auto formatted = format_html_clipboard(sender.buffer_);
    HANDLE object = GlobalAlloc(GMEM_MOVEABLE, formatted.size());
    if (!object)
      return;
    auto *buffer = (uint8_t*)GlobalLock(object);
    if (!buffer) {
      GlobalFree(object);
      return;
    }
    memcpy(buffer, formatted.data(), formatted.size());
    GlobalUnlock(object);
    SetClipboardData(_win_format, object);
  }
}

bool window::clipboard_receive(clipboard_formats _supported_formats, const receive_clipboard_data &_callback) {
  if (!OpenClipboard(window_))
    return false;
  for (auto format : _supported_formats) {
    auto wformat = display_.format_win32(format);
    if (wformat) {
      HANDLE object = GetClipboardData(wformat);
      auto *locked = GlobalLock(object);
      if (!locked) {
        CloseClipboard();
        return false;
      }
      switch(wformat) {
        case CF_UNICODETEXT: {
          auto *buffer = (WCHAR*)locked;
          auto utf8 = display::utf8_wstr(buffer);
          utf8.erase(std::remove(utf8.begin(), utf8.end(), '\r'), utf8.end());
          clipboard_receiver receiver{span<uint8_t>((uint8_t*)utf8.data(), utf8.size())};
          _callback(format, receiver);
        };
        case CF_DIB: {

        } break;
      }
      if (wformat == display_.html_clipboard_format_) {
        clipboard_receiver receiver{parse_html_clipboard((char*)locked)};
        _callback(format, receiver);
      }
      GlobalUnlock(object);
      CloseClipboard();
      return true;
    }
  }
  return false;
}

void window::interactive_resize(edge _edge) {
  if (!button_pressed_)
    return;

  WPARAM wparam = SC_SIZE;
  switch (_edge.active_) {
    case edge(TOP).active_: wparam += 3; break;
    case edge(BOTTOM).active_: wparam += 6; break;
    case edge(LEFT).active_: wparam += 1; break;
    case edge(RIGHT).active_: wparam += 2; break;

    case edge({TOP, LEFT}).active_: wparam += 4; break;
    case edge({TOP, RIGHT}).active_: wparam += 5; break;
    case edge({BOTTOM, LEFT}).active_: wparam += 7; break;
    case edge({BOTTOM, RIGHT}).active_: wparam += 8; break;
  }
  PostMessageA(window_, WM_SYSCOMMAND, wparam, 0);
}

void window::interactive_move() {
  if (!button_pressed_)
    return;

  constexpr WPARAM SC_DRAGMOVE = 0xf012;
  PostMessageA(window_, WM_SYSCOMMAND, SC_DRAGMOVE, 0);
}
