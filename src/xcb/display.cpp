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
#include <thread>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>
#include <xcb/xcb.h>

#include <glm/ext.hpp>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

display::display(const char *_app_name, uint32_t _app_version, const char *_name) {
  std::vector<const char *> extensions = {VK_KHR_XCB_SURFACE_EXTENSION_NAME};
  init_vulkan_instance(_app_name, _app_version, extensions);

  int screenNum;
  connection_ = xcb_connect(_name, &screenNum);
  const int result = xcb_connection_has_error(connection_);
  if (result > 0) {
    switch (result) {
      case XCB_CONN_ERROR:
        throw std::runtime_error("socket errors, pipe errors or other stream errors");
      case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
        throw std::runtime_error("extension not supported");
      case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
        throw std::runtime_error("memory not available");
      case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
        throw std::runtime_error("exceeding request length that server accepts");
      case XCB_CONN_CLOSED_PARSE_ERR:
        throw std::runtime_error("error during parsing display string");
      case XCB_CONN_CLOSED_INVALID_SCREEN:
        throw std::runtime_error("server does not have a screen matching the display");
      default:
        throw std::runtime_error("unknown xcb error");
    }
  }

  const xcb_setup_t *setup = xcb_get_setup(connection_);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  for (int i = 0; i < screenNum; i++)
    xcb_screen_next(&iter);
  screen_ = iter.data;

  keysyms_ = xcb_key_symbols_alloc(connection_);

  class atom_item_t {
   public:
    xcb_atom_t &ref_;
    xcb_intern_atom_cookie_t cookie_;

    atom_item_t(xcb_atom_t &_ref) : ref_(_ref) {
    }
  };

  std::map<std::string, atom_item_t> atoms = {
      {"WM_PROTOCOLS", {atom_wm_}}, {"WM_DELETE_WINDOW", {atom_close_}},
  };

  for (auto &atom : atoms)
    atom.second.cookie_ = xcb_intern_atom(connection_, 0, atom.first.size(), atom.first.data());

  for (auto &atom : atoms) {
    xcb_generic_error_t *error;
    auto reply = xcb_intern_atom_reply(connection_, atom.second.cookie_, &error);
    if (error) {
      throw std::runtime_error(sstream("Can't retreive atom ") << atom.first << ", code: " << error->error_code);
    }
    atom.second.ref_ = reply->atom;
    free(reply);
  }

  uint32_t mask = XCB_CW_BACK_PIXEL;
  uint32_t values[1] = {screen_->white_pixel};

  xcb_window_t dummy = xcb_generate_id(connection_);
  xcb_create_window(connection_, XCB_COPY_FROM_PARENT, dummy, screen_->root, 0, 0, 800, 600, 10,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, screen_->root_visual, mask, values);

  VkXcbSurfaceCreateInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
  info.connection = connection_;
  info.window = dummy;

  auto func = get_proc<PFN_vkCreateXcbSurfaceKHR>("vkCreateXcbSurfaceKHR");
  VkSurfaceKHR dummy_surface;
  VkResult vkr;
  if ((vkr = func(instance_, &info, nullptr, &dummy_surface)) != VK_SUCCESS)
    throw std::runtime_error(sstream("couldn't create dummy surface, code: ") << vkr);

  init_vulkan_device(dummy_surface);

  vkDestroySurfaceKHR(instance_, dummy_surface, nullptr);
  xcb_destroy_window(connection_, dummy);
}

display::~display() {
  destroy_vulkan();
  xcb_key_symbols_free(keysyms_);
  xcb_disconnect(connection_);
}

void display::flush() {
  xcb_flush(connection_);
}

void dispatch_keysym(display *thiz, window *w, xcb_keysym_t keysym, bool press) {
  switch (keysym) {
    case XK_KP_0:
      keysym = XK_0;
      break;
    case XK_KP_1:
      keysym = XK_1;
      break;
    case XK_KP_2:
      keysym = XK_2;
      break;
    case XK_KP_3:
      keysym = XK_3;
      break;
    case XK_KP_4:
      keysym = XK_4;
      break;
    case XK_KP_5:
      keysym = XK_5;
      break;
    case XK_KP_6:
      keysym = XK_6;
      break;
    case XK_KP_7:
      keysym = XK_7;
      break;
    case XK_KP_8:
      keysym = XK_8;
      break;
    case XK_KP_9:
      keysym = XK_9;
      break;
    case XK_KP_Add:
      keysym = XK_plus;
      break;
    case XK_KP_Divide:
      keysym = XK_slash;
      break;
    case XK_KP_Equal:
      keysym = XK_equal;
      break;
    case XK_KP_Multiply:
      keysym = XK_asterisk;
      break;
    case XK_KP_Subtract:
      keysym = XK_hyphen;
      break;
    case XK_KP_Begin:
      keysym = XK_Home;
      break;
    case XK_KP_End:
      keysym = XK_End;
      break;
    case XK_KP_Home:
      keysym = XK_Home;
      break;
    case XK_KP_Page_Up:
      keysym = XK_Page_Up;
      break;
    case XK_KP_Page_Down:
      keysym = XK_Page_Down;
      break;
    case XK_KP_Up:
      keysym = XK_Up;
      break;
    case XK_KP_Right:
      keysym = XK_Right;
      break;
    case XK_KP_Down:
      keysym = XK_Down;
      break;
    case XK_KP_Left:
      keysym = XK_Left;
      break;
    case XK_KP_Decimal:
      keysym = XK_period;
      break;
    case XK_KP_Separator:
      keysym = XK_apostrophe;
      break;
    case XK_KP_Enter:
      keysym = XK_Return;
      break;
    case XK_KP_Delete:
      keysym = XK_Delete;
      break;
    case XK_KP_F1:
      keysym = XK_F1;
      break;
    case XK_KP_F2:
      keysym = XK_F2;
      break;
    case XK_KP_F3:
      keysym = XK_F3;
      break;
    case XK_KP_F4:
      keysym = XK_F4;
      break;
    case XK_KP_Insert:
      keysym = XK_Insert;
      break;
    case XK_KP_Tab:
      keysym = XK_Tab;
      break;
  }

  thiz->post([w, keysym, press](auto) { w->on_keysym.fire(keysym, press); });
}

int display::dispatch() {
  if (windows_.empty())
    throw std::runtime_error("dispatch called without any window");

  bool loop = true;
  dispatcher_ = std::this_thread::get_id();

  std::thread event_pump([&loop, this]() {
    while (loop) {
      xcb_generic_event_t *event = xcb_wait_for_event(connection_);
      if (event != nullptr) {
        switch (event->response_type & ~0x80) {
          case XCB_EXPOSE: {
            xcb_expose_event_t *e = reinterpret_cast<xcb_expose_event_t *>(event);

            auto it = windows_.find(e->window);
            if (it != windows_.end()) {
              glm::uvec4 r{e->x, e->y, e->x + e->width, e->y + e->height};
              window *w = it->second;
              post_overridable(
                  [w, r](auto tp) {
                    w->on_expose.fire(r);  // FIXME JB: Rects should be merged
                    w->redraw(tp);
                  },
                  1);
            }
          } break;

          case XCB_CONFIGURE_NOTIFY: {
            xcb_configure_notify_event_t *e = reinterpret_cast<xcb_configure_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              glm::uvec2 s{e->width, e->height};
              if (s != w->size_) {
                post_overridable([w, s](auto) { w->dispatch_resize(s); }, 0);
              }
            }
          } break;

          case XCB_KEY_PRESS: {
            xcb_key_press_event_t *e = reinterpret_cast<xcb_key_press_event_t *>(event);

            int col;
            if (e->state & 0x3)
              col = 1;  // shift || caps-lock
            else if (e->state & 0x80)
              col = 4;  // alt-gr
            else
              col = 0;

            auto keysym = xcb_key_press_lookup_keysym(keysyms_, e, col);
            if (keysym >= 0xff80 && keysym <= 0xffb9 && keysym != XK_KP_Enter) {
              col = e->state & 0x10 ? 1 : 0;
              keysym = xcb_key_press_lookup_keysym(keysyms_, e, col);
            }

            auto it = windows_.find(e->event);
            if (it != windows_.end())
              dispatch_keysym(this, it->second, keysym, true);
          } break;

          case XCB_KEY_RELEASE: {
            xcb_key_press_event_t *e = reinterpret_cast<xcb_key_release_event_t *>(event);

            int col;
            if (e->state & 0x80)
              col = 4;  // alt-gr
            else if (e->state & 0x3)
              col = 1;  // shift || caps-lock
            else
              col = 0;

            auto keysym = xcb_key_press_lookup_keysym(keysyms_, e, col);
            if (keysym >= 0xff80 && keysym <= 0xffb9 && keysym != XK_KP_Enter) {
              col = e->state & 0x10 ? 1 : 0;
              keysym = xcb_key_press_lookup_keysym(keysyms_, e, col);
            }

            auto it = windows_.find(e->event);
            if (it != windows_.end())
              dispatch_keysym(this, it->second, keysym, false);
          } break;

          case XCB_BUTTON_PRESS: {
            xcb_button_press_event_t *e = reinterpret_cast<xcb_button_press_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              uint8_t b = e->detail;
              auto c = glm::vec2{e->event_x, e->event_y};
              mouse_event_type t;
              switch (e->detail) {
                case 4:
                  t = mouse_event_type::MWHEEL_UP;
                  break;
                case 5:
                  t = mouse_event_type::MWHEEL_DOWN;
                  break;
                default:
                  t = mouse_event_type::MDOWN;
                  break;
              }
              post([w, b, t, c](auto) { w->on_mouse.fire(b, t, c); });
            }
          } break;

          case XCB_BUTTON_RELEASE: {
            xcb_button_release_event_t *e = reinterpret_cast<xcb_button_release_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              uint8_t b = e->detail;
              if (b < 4 || b > 5) {  // ignore wheel events
                auto c = glm::vec2{e->event_x, e->event_y};
                post([w, b, c](auto) { w->on_mouse.fire(b, mouse_event_type::MUP, c); });
              }
            }
          } break;

          case XCB_ENTER_NOTIFY: {
            xcb_enter_notify_event_t *e = reinterpret_cast<xcb_enter_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              auto c = glm::vec2{e->event_x, e->event_y};
              uint8_t b = e->detail;
              post([w, b, c](auto) { w->on_mouse.fire(b, mouse_event_type::MENTER, c); });
            }
          } break;

          case XCB_LEAVE_NOTIFY: {
            xcb_leave_notify_event_t *e = reinterpret_cast<xcb_leave_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              auto c = glm::vec2{e->event_x, e->event_y};
              uint8_t b = e->detail;
              post([w, b, c](auto) { w->on_mouse.fire(b, mouse_event_type::MLEAVE, c); });
            }
          } break;

          case XCB_MOTION_NOTIFY: {
            xcb_motion_notify_event_t *e = reinterpret_cast<xcb_motion_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              auto c = glm::vec2{e->event_x, e->event_y};
              uint8_t b = e->detail;
              post([w, b, c](auto) { w->on_mouse.fire(b, mouse_event_type::MMOVE, c); });
            }
          } break;

          case XCB_CLIENT_MESSAGE: {
            xcb_client_message_event_t *e = reinterpret_cast<xcb_client_message_event_t *>(event);

            auto it = windows_.find(e->window);
            if (it != windows_.end()) {
              if (e->data.data32[0] == atom_close_) {
                window *w = it->second;
                post([w](auto) {
                  if (!w->on_close.fire())
                    w->close();
                });
              }
            }
          } break;

          case XCB_FOCUS_IN: {
            xcb_focus_in_event_t *e = reinterpret_cast<xcb_focus_in_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              post([w](auto) { w->on_focus.fire(); });
            }
          } break;

          case XCB_FOCUS_OUT: {
            xcb_focus_out_event_t *e = reinterpret_cast<xcb_focus_out_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              post([w](auto) { w->on_blur.fire(); });
            }
          } break;

          case XCB_MAP_NOTIFY: {
            xcb_map_notify_event_t *e = reinterpret_cast<xcb_map_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              post([w](auto) { w->on_resume.fire(); });
            }
          } break;

          case XCB_UNMAP_NOTIFY: {
            xcb_unmap_notify_event_t *e = reinterpret_cast<xcb_unmap_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              post([w](auto) { w->on_pause.fire(); });
            }
          } break;

          case XCB_DESTROY_NOTIFY: {
            if (windows_.empty()) {
              loop = false;
              cv_.notify_one();
            }
          } break;

          default:
            break;
        }
      }

      free(event);
    }
  });

  while (loop) {
    jobs_loop();
  }

  event_pump.join();

  return EXIT_SUCCESS;
}
