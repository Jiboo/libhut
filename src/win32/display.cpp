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

#include <malloc.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <set>

#include <windows.h>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

ssize_t clipboard_sender::write(span<uint8_t> _data) {
  buffer_.insert(buffer_.cend(), _data.begin(), _data.end());
  return _data.size_bytes();
}

ssize_t clipboard_receiver::read(span<uint8_t> _data) {
  size_t remaining = buffer_.size() - offset_;
  size_t reading = std::min(remaining, _data.size());
  memcpy(_data.data(), buffer_.data() + offset_, reading);
  offset_ += reading;
  return reading;
}

std::string display::utf8_wstr(const WCHAR *_input) {
#ifdef WC_ERR_INVALID_CHARS
  DWORD flags = WC_ERR_INVALID_CHARS;
#else
  DWORD flags = 0;
#endif

  auto size = WideCharToMultiByte(CP_UTF8, flags, _input, -1, NULL, 0, NULL, NULL);
  if (size <= 0) return "";

  std::string result;
  result.resize(size);

  if (!WideCharToMultiByte(CP_UTF8, flags, _input, -1, result.data(), size, NULL, NULL))
    return "";

  return result;
}

UINT display::format_win32(clipboard_format _format) {
  switch (_format) {
    case FTEXT_PLAIN: return CF_UNICODETEXT;
    case FIMAGE_BMP: return CF_BITMAP;
    case FTEXT_HTML: return html_clipboard_format_;
    default: return 0;
  }
}

std::optional<clipboard_format> display::win32_format(UINT _format) {
  switch(_format) {
    case CF_UNICODETEXT: return FTEXT_PLAIN;
    case CF_BITMAP: return FIMAGE_BMP;
  }
  if (_format == html_clipboard_format_)
    return FTEXT_HTML;
  return {};
}

display::display(const char *_app_name, uint32_t _app_version, const char *) {
  std::vector<const char *> extensions = {VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
  init_vulkan_instance(_app_name, _app_version, extensions);

  hinstance_ = GetModuleHandle(nullptr);

  // Register window class
  WNDCLASSEXA wc_ssd = {};
  wc_ssd.cbSize = sizeof(wc_ssd);
  wc_ssd.lpszClassName = HUT_WIN32_CLASSNAME_SSD;
  wc_ssd.hInstance = hinstance_;
  wc_ssd.lpfnWndProc = hut::WindowProc;
  wc_ssd.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc_ssd.hCursor = LoadCursorA(nullptr, IDC_ARROW);
  if (!RegisterClassExA(&wc_ssd)) {
    throw std::runtime_error("couldn't register window ssd class");
  }

  WNDCLASSEXA wc_csd = {};
  wc_csd.cbSize = sizeof(wc_csd);
  wc_csd.lpszClassName = HUT_WIN32_CLASSNAME_CSD;
  wc_csd.hInstance = hinstance_;
  wc_csd.lpfnWndProc = hut::WindowProc;
  wc_csd.style = 0;
  wc_csd.hCursor = LoadCursorA(nullptr, IDC_ARROW);
  if (!RegisterClassExA(&wc_csd)) {
    throw std::runtime_error("couldn't register window csd class");
  }

  // Dummy window to request surface capabilities
  HWND dummy = CreateWindowExA(WS_EX_OVERLAPPEDWINDOW, HUT_WIN32_CLASSNAME_SSD, "Dummy", WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                               0, 0, 100, 100, nullptr, nullptr, hinstance_, this);
  if (!dummy) {
    throw std::runtime_error("couldn't create window");
  }

  VkWin32SurfaceCreateInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  info.hinstance = hinstance_;
  info.hwnd = dummy;

  auto func = get_proc<PFN_vkCreateWin32SurfaceKHR>("vkCreateWin32SurfaceKHR");
  VkSurfaceKHR dummy_surface;
  VkResult vkr;
  if ((vkr = func(instance_, &info, nullptr, &dummy_surface)) != VK_SUCCESS)
    throw std::runtime_error(sstream("couldn't create dummy surface, code: ") << vkr);

  init_vulkan_device(dummy_surface);

  HUT_PVK(vkDestroySurfaceKHR, instance_, dummy_surface, nullptr);
  DestroyWindow(dummy);

  FT_Init_FreeType(&ft_library_);

  html_clipboard_format_ = RegisterClipboardFormatA("HTML Format");
}

display::~display() {
  UnregisterClassA(HUT_WIN32_CLASSNAME_CSD, hinstance_);
  UnregisterClassA(HUT_WIN32_CLASSNAME_SSD, hinstance_);
  FT_Done_FreeType(ft_library_);
  destroy_vulkan();
}

void display::flush() {
}

void display::post_empty_event() {
  if (windows_.empty())
    return;

  PostMessage(windows_.begin()->first, WM_NULL, 0, 0);
}

keysym get_keysym(WPARAM _wparam, LPARAM _lparam) {
  const bool extended = (_lparam & 0x100) == 0x100;
  switch (_wparam) {
    case VK_ESCAPE: return KSYM_ESC;
    case '1': return KSYM_1;
    case '2': return KSYM_2;
    case '3': return KSYM_3;
    case '4': return KSYM_4;
    case '5': return KSYM_5;
    case '6': return KSYM_6;
    case '7': return KSYM_7;
    case '8': return KSYM_8;
    case '9': return KSYM_9;
    case '0': return KSYM_0;
    case VK_SUBTRACT: return KSYM_MINUS;
    //case VK_EQUAL: return KSYM_EQUAL;
    case VK_BACK: return KSYM_BACKSPACE;
    case VK_TAB: return KSYM_TAB;
    case 'Q': return KSYM_Q;
    case 'W': return KSYM_W;
    case 'E': return KSYM_E;
    case 'R': return KSYM_R;
    case 'T': return KSYM_T;
    case 'Y': return KSYM_Y;
    case 'U': return KSYM_U;
    case 'I': return KSYM_I;
    case 'O': return KSYM_O;
    case 'P': return KSYM_P;
    /*case XKB_KEY_braceleft: return KSYM_LEFTBRACE;
    case XKB_KEY_braceright: return KSYM_RIGHTBRACE;*/
    case VK_RETURN: return KSYM_ENTER;
    case VK_CONTROL: return extended ? KSYM_RIGHTCTRL : KSYM_LEFTCTRL;
    case 'A': return KSYM_A;
    case 'S': return KSYM_S;
    case 'D': return KSYM_D;
    case 'F': return KSYM_F;
    case 'G': return KSYM_G;
    case 'H': return KSYM_H;
    case 'J': return KSYM_J;
    case 'K': return KSYM_K;
    case 'L': return KSYM_L;
    /*case XKB_KEY_semicolon: return KSYM_SEMICOLON;
    case XKB_KEY_apostrophe: return KSYM_APOSTROPHE;
    case XKB_KEY_grave: return KSYM_GRAVE;*/
    case VK_SHIFT: return extended ? KSYM_RIGHTSHIFT : KSYM_LEFTSHIFT;
    //case XKB_KEY_backslash: return KSYM_BACKSLASH;
    case 'Z': return KSYM_Z;
    case 'X': return KSYM_X;
    case 'C': return KSYM_C;
    case 'V': return KSYM_V;
    case 'B': return KSYM_B;
    case 'N': return KSYM_N;
    case 'M': return KSYM_M;
    /*case XKB_KEY_comma: return KSYM_COMMA;
    case XKB_KEY_period: return KSYM_DOT;
    case XKB_KEY_slash: return KSYM_SLASH;
    case XKB_KEY_KP_Multiply: return KSYM_KPASTERISK;*/
    case VK_MENU: return extended ? KSYM_RIGHTALT : KSYM_LEFTALT;
    case VK_SPACE: return KSYM_SPACE;
    case VK_CAPITAL: return KSYM_CAPSLOCK;
    case VK_F1: return KSYM_F1;
    case VK_F2: return KSYM_F2;
    case VK_F3: return KSYM_F3;
    case VK_F4: return KSYM_F4;
    case VK_F5: return KSYM_F5;
    case VK_F6: return KSYM_F6;
    case VK_F7: return KSYM_F7;
    case VK_F8: return KSYM_F8;
    case VK_F9: return KSYM_F9;
    case VK_F10: return KSYM_F10;
    case VK_F11: return KSYM_F11;
    case VK_F12: return KSYM_F12;
    case VK_NUMLOCK: return KSYM_NUMLOCK;
    case VK_SCROLL: return KSYM_SCROLLLOCK;
    /*case XKB_KEY_KP_7: return KSYM_KP7;
    case XKB_KEY_KP_8: return KSYM_KP8;
    case XKB_KEY_KP_9: return KSYM_KP9;
    case XKB_KEY_KP_Subtract: return KSYM_KPMINUS;
    case XKB_KEY_KP_4: return KSYM_KP4;
    case XKB_KEY_KP_5: return KSYM_KP5;
    case XKB_KEY_KP_6: return KSYM_KP6;
    case XKB_KEY_KP_Add: return KSYM_KPPLUS;
    case XKB_KEY_KP_1: return KSYM_KP1;
    case XKB_KEY_KP_2: return KSYM_KP2;
    case XKB_KEY_KP_3: return KSYM_KP3;
    case XKB_KEY_KP_0: return KSYM_KP0;
    case XKB_KEY_KP_Decimal: return KSYM_KPDOT;
    case XKB_KEY_KP_Enter: return KSYM_KPENTER;
    case XKB_KEY_KP_Divide: return KSYM_KPSLASH;*/
    case VK_SNAPSHOT: return KSYM_SYSRQ;
    //case XKB_KEY_Linefeed: return KSYM_LINEFEED;
    case VK_HOME: return KSYM_HOME;
    case VK_UP: return KSYM_UP;
    case VK_PRIOR: return KSYM_PAGEUP;
    case VK_LEFT: return KSYM_LEFT;
    case VK_RIGHT: return KSYM_RIGHT;
    case VK_END: return KSYM_END;
    case VK_DOWN: return KSYM_DOWN;
    case VK_NEXT: return KSYM_PAGEDOWN;
    case VK_INSERT: return KSYM_INSERT;
    case VK_DELETE: return KSYM_DELETE;
    //case XKB_KEY_KP_Equal: return KSYM_KPEQUAL;
    case VK_PAUSE: return KSYM_PAUSE;
    case VK_LWIN: return KSYM_LEFTMETA;
    case VK_RWIN: return KSYM_RIGHTMETA;
    default:
      //std::cout << "Invalid char in get_keysym: " << _wparam << ", " << (int)_wparam << std::endl;
      return KSYM_INVALID;
  }
}

keycode get_keycode(LPARAM _lparam) {
  const UINT ulparam = _lparam;
  const UINT scancode = (ulparam & 0x01ff0000U) >> 16U;
  return scancode;
}

char32_t display::keycode_idle_char(keycode _in) const {
  const auto vsc = MapVirtualKeyA(_in, MAPVK_VSC_TO_VK_EX);
  return to_utf32(MapVirtualKeyA(vsc, MAPVK_VK_TO_CHAR));
}

char *display::keycode_name(span<char> _out, keycode _in) const {
  return _out.data() + GetKeyNameTextA(_in << 16U, _out.data(), _out.size());
}

LRESULT CALLBACK hut::WindowProc(HWND _hwnd, UINT _umsg, WPARAM _wparam, LPARAM _lparam) {
  window *w = nullptr;
  auto   *d = (display*)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
  if (d) {
    const auto &windows = d->windows_;
    auto it = windows.find(_hwnd);
    if (it != windows.end()) {
      assert(_hwnd == it->first);
      w = it->second;
    }
  }

  switch (_umsg) {
    case WM_CREATE: {
      SetWindowLongPtr(_hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT *)_lparam)->lpCreateParams);
      return 1;
    }

    case WM_PAINT: {
      if (w) {
        if (!IsIconic(w->window_)) {
          uvec4 r{ 0, 0, w->size_ };
          PAINTSTRUCT paint;
          BeginPaint(_hwnd, &paint);
          w->on_expose.fire(r);
          w->redraw(display::clock::now());
          EndPaint(_hwnd, &paint);
        }
      }
      return 0;
    }

    case WM_MOVE: {
      if (w) {
        w->pos_ = {LOWORD(_lparam), HIWORD(_lparam)};
      }
      return 0;
    }

    case WM_SIZE: {
      if (w) {
        switch (_wparam) {
        case SIZE_RESTORED:
          if (w->minimized_) {
            w->minimized_ = false;
            w->on_resume.fire();
          }
          break;
        case SIZE_MINIMIZED:
          w->minimized_ = true;
          w->on_pause.fire();
          break;
        default:
          break;
        }
        uvec2 s{LOWORD(_lparam), HIWORD(_lparam)};
        if (s != w->size_) {
          w->size_ = s;
          w->init_vulkan_surface();
          w->on_resize.fire(s);
        }
      }
      return 0;
    }

    case WM_CLOSE: {
      if (w) {
        if (!w->on_close.fire())
          w->close();
      }
      return 0;
    }

    case WM_MOUSEWHEEL: {
      if (w) {
        vec2 pos{LOWORD(_lparam), HIWORD(_lparam)};
        auto speed = static_cast<int16_t>(HIWORD(_wparam));
        w->on_mouse.fire(0, speed >= 0 ? MWHEEL_UP : MWHEEL_DOWN, pos);
      }
      return 0;
    }

    case WM_XBUTTONDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:{
      if (w) {
        uint8_t button = 1;
        switch(_umsg) {
          case WM_LBUTTONDOWN: w->button_pressed_ = true; break;
          case WM_RBUTTONDOWN: button = 2; break;
          case WM_MBUTTONDOWN: button = 3; break;
          case WM_XBUTTONDOWN: button = (HIWORD(_wparam) == XBUTTON1 ? 4 : 5); break;
        }
        vec2 pos{LOWORD(_lparam), HIWORD(_lparam)};
        w->on_mouse.fire(button, MDOWN, pos);
      }
      return 0;
    }

    case WM_XBUTTONUP:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
      if (w) {
        uint8_t button = 1;
        switch(_umsg) {
          case WM_LBUTTONUP: w->button_pressed_ = false; break;
          case WM_RBUTTONUP: button = 2; break;
          case WM_MBUTTONUP: button = 3; break;
          case WM_XBUTTONUP: button = (HIWORD(_wparam) == XBUTTON1 ? 4 : 5); break;
        }
        vec2 pos{LOWORD(_lparam), HIWORD(_lparam)};
        w->on_mouse.fire(button, MUP, pos);
      }
      return 0;
    }

    case WM_MOUSEMOVE: {
      if (w) {
        uint8_t button = 0;
        if (_wparam & MK_RBUTTON)
          button = 1;
        else if (_wparam & MK_MBUTTON)
          button = 2;
        vec2 pos{LOWORD(_lparam), HIWORD(_lparam)};
        w->on_mouse.fire(button, MMOVE, pos);
      }
      return 0;
    }

    case WM_SETFOCUS: {
      if (w) {
        w->on_focus.fire();
      }
      return 0;
    }

    case WM_KILLFOCUS: {
      if (w) {
        w->on_blur.fire();
      }
      return 0;
    }

    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_KEYDOWN: {
      if (w) {
        keysym ksym = get_keysym(_wparam, _lparam);
        if (ksym == KSYM_INVALID)
          return 0;

        keycode kcode = get_keycode(_lparam);
        bool pressed = _umsg == WM_KEYDOWN || _umsg == WM_SYSKEYDOWN;
        if ((ksym == KSYM_LEFTALT || ksym == KSYM_RIGHTALT) && _umsg == WM_SYSKEYUP)
          return 0; // Alt up would send both WM_SYSKEYUP and WM_KEYUP

        w->on_key.fire(kcode, ksym, pressed);

        const bool alt = ksym == KSYM_LEFTALT || ksym == KSYM_RIGHTALT;
        const bool ctrl = ksym == KSYM_LEFTCTRL || ksym == KSYM_RIGHTCTRL;
        const bool shift = ksym == KSYM_LEFTSHIFT || ksym == KSYM_RIGHTSHIFT;
        if (alt || ctrl || shift) {
          modifiers mods;
          auto is_pressed = [](int _vkey) {
            return static_cast<uint16_t>(GetAsyncKeyState(_vkey)) & 0x8000U;
          };
          if (is_pressed(VK_MENU)) mods.set(KMOD_ALT);
          if (is_pressed(VK_CONTROL)) mods.set(KMOD_CTRL);
          if (is_pressed(VK_SHIFT)) mods.set(KMOD_SHIFT);
          w->on_kmods.fire(mods);
        }

        return 0;
      }
      return 1;
    }

    case WM_CHAR: {
      if (w) {
        char32_t c = to_utf32(_wparam);
        if (c >= 0x20)
          w->on_char.fire(c);
      }
      return 0;
    }

    case WM_SETCURSOR: {
      if (w) {
        w->cursor(w->current_cursor_type_);
      }
      return 0;
    }

    case WM_RENDERFORMAT: {
      if (w) {
        auto format = d->win32_format(_wparam);
        if (format)
          w->clipboard_write(*format, _wparam);
      }
    }

    case WM_RENDERALLFORMATS: {
      if (w) {
        for (auto format : w->current_clipboard_formats_)
          w->clipboard_write(format, d->format_win32((format)));
      }
    }

    default:
      break;
  }
  return DefWindowProcA(_hwnd, _umsg, _wparam, _lparam);
}

int display::dispatch() {
  if (windows_.empty())
    throw std::runtime_error("dispatch called without any window");

  SetConsoleOutputCP(65001);

  bool loop = true;
  while (loop) {
    MSG msg;
    auto result = GetMessageA(&msg, nullptr, 0, 0);
    do {
      if (result < 0 || msg.message == WM_QUIT) {
        loop = false;
      } else {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);

        if (windows_.empty()) {
          PostQuitMessage(0);
          loop = false;
        }
      }
      result = PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE);
    } while(result != 0);
    process_posts(display::clock::now());
  }

  return EXIT_SUCCESS;
}
