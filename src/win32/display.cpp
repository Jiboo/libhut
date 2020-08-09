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
#include <thread>

#include <windows.h>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

std::string display::utf8_wstr(const WCHAR *_input) {
  size_t size = WideCharToMultiByte(CP_UTF8, 0, _input, -1, NULL, 0, NULL, NULL);
  if (!size) return "";

  std::string result;
  result.resize(size);

  if (!WideCharToMultiByte(CP_UTF8, 0, _input, -1, result.data(), size, NULL, NULL))
    return "";

  return result;
}

UINT display::format_win32(clipboard_format _format) {
  switch (_format) {
    case FTEXT_PLAIN: return CF_UNICODETEXT;
    case FIMAGE_BMP: return CF_BITMAP;
    case FTEXT_HTML: return html_clipboard_format_;
  }
  return 0;
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

  vkDestroySurfaceKHR(instance_, dummy_surface, nullptr);
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

keysym map_key(WPARAM _c) {
  switch (_c) {
    case VK_TAB: return KTAB;
    case VK_LMENU: return KALT_LEFT;
    case VK_RMENU: return KALT_RIGHT;
    case VK_LCONTROL: return KCTRL_LEFT;
    case VK_RCONTROL: return KCTRL_RIGHT;
    case VK_LSHIFT: return KSHIFT_LEFT;
    case VK_RSHIFT: return KSHIFT_RIGHT;
    case VK_PRIOR: return KPAGE_UP;
    case VK_NEXT: return KPAGE_DOWN;
    case VK_UP: return KUP;
    case VK_RIGHT: return KRIGHT;
    case VK_DOWN: return KDOWN;
    case VK_LEFT: return KLEFT;
    case VK_HOME: return KHOME;
    case VK_END: return KEND;
    case VK_RETURN: return KRETURN;
    case VK_BACK: return KBACKSPACE;
    case VK_DELETE: return KDELETE;
    case VK_INSERT: return KINSER;
    case VK_ESCAPE: return KESCAPE;
#define HUT_MAPFUNCKEY(NUM) case VK_F##NUM: return KF##NUM
    HUT_MAPFUNCKEY(1);
    HUT_MAPFUNCKEY(2);
    HUT_MAPFUNCKEY(3);
    HUT_MAPFUNCKEY(4);
    HUT_MAPFUNCKEY(5);
    HUT_MAPFUNCKEY(6);
    HUT_MAPFUNCKEY(7);
    HUT_MAPFUNCKEY(8);
    HUT_MAPFUNCKEY(9);
    HUT_MAPFUNCKEY(10);
    HUT_MAPFUNCKEY(11);
    HUT_MAPFUNCKEY(12);
#undef HUT_MAPFUNCKEY
    default:
      return keysym(char32_t(MapVirtualKeyA(_c, MAPVK_VK_TO_CHAR)));
  }
}

// https://stackoverflow.com/questions/15966642/how-do-you-tell-lshift-apart-from-rshift-in-wm-keydown-events
WPARAM map_controlkeys(WPARAM _vk, LPARAM _lparam) {
  const UINT ulparam = _lparam;
  const UINT scancode = (ulparam & 0x00ff0000U) >> 16U;
  const int extended = (ulparam & 0x01000000U) != 0;

  switch (_vk) {
    case VK_SHIFT: return MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
    case VK_CONTROL: return extended ? VK_RCONTROL : VK_LCONTROL;
    case VK_MENU: return extended ? VK_RMENU : VK_LMENU;
    default:
      return _vk;
  }
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
        keysym k = map_key(map_controlkeys(_wparam, _lparam));
        if ((k == KALT_LEFT || k == KALT_RIGHT) && _umsg == WM_SYSKEYUP)
          return 0; // Alt up would send both WM_SYSKEYUP and WM_KEYUP
        const bool pressed = _umsg == WM_KEYDOWN || _umsg == WM_SYSKEYDOWN;
        const bool alt = GetKeyState(VK_MENU);
        const bool ctrl = GetKeyState(VK_CONTROL);
        const bool shift = GetKeyState(VK_SHIFT);
        auto mods = modifiers{ (alt ? KMOD_ALT : 0U) | (ctrl ? KMOD_CTRL : 0U) | (shift ? KMOD_SHIFT : 0U) };
        w->on_key.fire(k, mods, pressed);
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
        w->set_cursor(w->current_cursor_type_);
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
