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

#include <glm/ext.hpp>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

display::display(const char *_app_name, uint32_t _app_version, const char *) {
  std::vector<const char *> extensions = {VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
  init_vulkan_instance(_app_name, _app_version, extensions);

  hinstance_ = GetModuleHandle(NULL);

  // Register window class
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc = hut::WindowProc;
  wc.hInstance = hinstance_;
  wc.lpszClassName = HUT_WIN32_CLASSNAME;
  if (!RegisterClassExW(&wc)) {
    throw std::runtime_error("couldn't register window class");
  }

  // Dummy window to request surface capabilities
  HWND dummy = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, HUT_WIN32_CLASSNAME, L"Dummy", WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                               0, 0, 800, 600, NULL, NULL, hinstance_, this);
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
}

display::~display() {
  UnregisterClassW(HUT_WIN32_CLASSNAME, hinstance_);
  FT_Done_FreeType(ft_library_);
  destroy_vulkan();
}

void display::flush() {
}

void display::post_empty_event() {
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
      return keysym(char32_t(MapVirtualKeyW(_c, MAPVK_VK_TO_CHAR)));
  }
}

// https://stackoverflow.com/questions/15966642/how-do-you-tell-lshift-apart-from-rshift-in-wm-keydown-events
WPARAM map_controlkeys(WPARAM _vk, LPARAM _lparam) {
  UINT scancode = (_lparam & 0x00ff0000) >> 16;
  int extended = (_lparam & 0x01000000) != 0;

  switch (_vk) {
    case VK_SHIFT: return MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
    case VK_CONTROL: return extended ? VK_RCONTROL : VK_LCONTROL;
    case VK_MENU: return extended ? VK_RMENU : VK_LMENU;
    default:
      return _vk;
  }
}

LRESULT CALLBACK hut::WindowProc(HWND _hwnd, UINT _umsg, WPARAM _wparam, LPARAM _lparam) {
  switch (_umsg) {
    case WM_CREATE: {
      SetWindowLongPtr(_hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT *)_lparam)->lpCreateParams);
      return TRUE;
    } break;

    case WM_PAINT: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        uvec4 r{0, 0, w->size_};
        PAINTSTRUCT paint;
        BeginPaint(_hwnd, &paint);
        w->on_expose.fire(r);
        w->redraw(display::clock::now());
        EndPaint(_hwnd, &paint);
      }
      return 0;
    } break;

    case WM_SIZE: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        uvec2 s{LOWORD(_lparam), HIWORD(_lparam)};
        if (s != w->size_) {
          w->dispatch_resize(s);
        }
      }
      return 0;
    } break;

    case WM_CLOSE: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        if (!w->on_close.fire())
          w->close();
      }
      return 0;
    } break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        uint8_t button = 0;
        if (_umsg == WM_RBUTTONDOWN)
          button = 1;
        else if (_umsg == WM_MBUTTONDOWN)
          button = 2;
        uvec2 pos{LOWORD(_lparam), HIWORD(_lparam)};
        w->on_mouse.fire(button, MDOWN, pos);
      }
      return 0;
    } break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        uint8_t button = 0;
        if (_umsg == WM_RBUTTONUP)
          button = 1;
        else if (_umsg == WM_MBUTTONUP)
          button = 2;
        uvec2 pos{LOWORD(_lparam), HIWORD(_lparam)};
        w->on_mouse.fire(button, MUP, pos);
      }
      return 0;
    } break;

    case WM_MOUSEMOVE: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        uint8_t button = 0;
        if (_wparam & MK_RBUTTON)
          button = 1;
        else if (_wparam & MK_MBUTTON)
          button = 2;
        uvec2 pos{LOWORD(_lparam), HIWORD(_lparam)};
        w->on_mouse.fire(button, MMOVE, pos);
      }
      return 0;
    } break;

    case WM_SETFOCUS: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        w->on_focus.fire();
      }
      return 0;
    } break;

    case WM_KILLFOCUS: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        w->on_blur.fire();
      }
      return 0;
    } break;

    case WM_SYSCOMMAND: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        switch (_wparam) {
          case SC_MINIMIZE:
            w->on_pause.fire();
            break;
          case SC_RESTORE:
            w->on_resume.fire();
            break;
        }
      }
      return DefWindowProcW(_hwnd, _umsg, _wparam, _lparam);
    } break;

    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_KEYDOWN: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        keysym k = map_key(map_controlkeys(_wparam, _lparam));
        bool pressed = _umsg == WM_KEYDOWN;
        w->on_key.fire(k, pressed);
        return 0;
      }
      return 1;
    } break;

    case WM_CHAR: {
      display *d = (display *)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
      assert(d);
      auto it = d->windows_.find(_hwnd);
      if (it != d->windows_.end()) {
        window *w = it->second;
        char32_t c = to_utf32(_wparam);
        w->on_char.fire(c);
      }
      return 0;
    } break;
  }
  return DefWindowProcW(_hwnd, _umsg, _wparam, _lparam);
}

int display::dispatch() {
  if (windows_.empty())
    throw std::runtime_error("dispatch called without any window");

  SetConsoleOutputCP(65001);

  bool loop = true;
  while (loop) {
    MSG msg;
    auto result = GetMessageW(&msg, nullptr, 0, 0);
    process_posts(display::clock::now());
    if (result < 0 || msg.message == WM_QUIT) {
      loop = false;
    } else {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);

      if (windows_.empty()) {
        PostQuitMessage(0);
        loop = false;
      }
    }
  }

  return EXIT_SUCCESS;
}
