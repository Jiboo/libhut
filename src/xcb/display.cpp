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
#include <xcb/xcb_event.h>

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
    xcb_intern_atom_cookie_t cookie_ = {0};

    explicit atom_item_t(xcb_atom_t &_ref) : ref_(_ref) {
    }
  };

  std::map<std::string, atom_item_t> atoms = {
      {"WM_PROTOCOLS",                 atom_item_t{atom_wm_}},
      {"WM_DELETE_WINDOW",             atom_item_t{atom_close_}},
      {"WM_CHANGE_STATE",              atom_item_t{atom_change_state_}},
      {"_NET_WM_STATE",                atom_item_t{atom_state_}},
      {"_NET_WM_STATE_REMOVE",         atom_item_t{atom_rstate_}},
      {"_NET_WM_STATE_HIDDEN",         atom_item_t{atom_hstate_}},
      {"_NET_WM_STATE_MAXIMIZED_VERT", atom_item_t{atom_maximizeh_}},
      {"_NET_WM_STATE_MAXIMIZED_HORZ", atom_item_t{atom_maximizev_}},
      {"_MOTIF_WM_HINTS",              atom_item_t{atom_window_hints_}},
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
  xcb_create_window(connection_, XCB_COPY_FROM_PARENT, dummy, screen_->root, 0, 0, 800, 600, 0,
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

  FT_Init_FreeType(&ft_library_);

  if (int error = xcb_cursor_context_new(connection_, screen_, &cursor_context_) < 0)
    throw std::runtime_error(sstream("couldn't create cursor context: ") << error);
}

display::~display() {
  destroy_vulkan();

  if (ft_library_) FT_Done_FreeType(ft_library_);
  if (cursor_context_) xcb_cursor_context_free(cursor_context_);
  if (keysyms_) xcb_key_symbols_free(keysyms_);
  if (connection_) xcb_disconnect(connection_);
}

void display::flush() {
  xcb_flush(connection_);
}

void display::post_empty_event() {
  xcb_client_message_event_t event;
  event.response_type = XCB_CLIENT_MESSAGE;
  event.format = 32;

  xcb_send_event(connection_, 0, screen_->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (const char *)&event);
  xcb_flush(connection_);
}

xcb_keysym_t clean_kp_keysyms(xcb_keysym_t _input) {
  switch (_input) {
    case XK_KP_0: return XK_0;
    case XK_KP_1: return XK_1;
    case XK_KP_2: return XK_2;
    case XK_KP_3: return XK_3;
    case XK_KP_4: return XK_4;
    case XK_KP_5: return XK_5;
    case XK_KP_6: return XK_6;
    case XK_KP_7: return XK_7;
    case XK_KP_8: return XK_8;
    case XK_KP_9: return XK_9;
    case XK_KP_Add: return XK_plus;
    case XK_KP_Divide: return XK_slash;
    case XK_KP_Equal: return XK_equal;
    case XK_KP_Multiply: return XK_asterisk;
    case XK_KP_Subtract: return XK_hyphen;
    case XK_KP_Begin: return XK_Home;
    case XK_KP_End: return XK_End;
    case XK_KP_Home: return XK_Home;
    case XK_KP_Page_Up: return XK_Page_Up;
    case XK_KP_Page_Down: return XK_Page_Down;
    case XK_KP_Up: return XK_Up;
    case XK_KP_Right: return XK_Right;
    case XK_KP_Down: return XK_Down;
    case XK_KP_Left: return XK_Left;
    case XK_KP_Enter: return XK_Return;
    case XK_KP_Delete: return XK_Delete;
    case XK_KP_F1: return XK_F1;
    case XK_KP_F2: return XK_F2;
    case XK_KP_F3: return XK_F3;
    case XK_KP_F4: return XK_F4;
    case XK_KP_Insert: return XK_Insert;
    case XK_KP_Tab: return XK_Tab;
    default: return _input;
  }
}

keysym map_hut_enum(xcb_keysym_t _input) {
  switch (_input) {
    case XK_Tab: return KTAB;
    case XK_Alt_L: return KALT_LEFT;
    case 0xfe03: // altgr
    case XK_Alt_R: return KALT_RIGHT;
    case XK_Control_L: return KCTRL_LEFT;
    case XK_Control_R: return KCTRL_RIGHT;
    case XK_Shift_L: return KSHIFT_LEFT;
    case XK_Shift_R: return KSHIFT_RIGHT;
    case XK_Page_Up: return KPAGE_UP;
    case XK_Page_Down: return KPAGE_DOWN;
    case XK_Up: return KUP;
    case XK_Right: return KRIGHT;
    case XK_Down: return KDOWN;
    case XK_Left: return KLEFT;
    case XK_Home: return KHOME;
    case XK_End: return KEND;
    case XK_Return: return KRETURN;
    case XK_BackSpace: return KBACKSPACE;
    case XK_Delete: return KDELETE;
    case XK_Insert: return KINSER;
    case XK_Escape: return KESCAPE;
    case XK_F1: return KF1;
    case XK_F2: return KF2;
    case XK_F3: return KF3;
    case XK_F4: return KF4;
    case XK_F5: return KF5;
    case XK_F6: return KF6;
    case XK_F7: return KF7;
    case XK_F8: return KF8;
    case XK_F9: return KF9;
    case XK_F10: return KF10;
    case XK_F11: return KF11;
    case XK_F12: return KF12;
    default: return (keysym)_input;
  }
}

void dispatch_keysym(window *w, uint16_t _mask, xcb_keysym_t _ks, bool press) {
  const xcb_keysym_t cleaned = clean_kp_keysyms(_ks);
  const keysym mapped = map_hut_enum(cleaned);
  const bool remapped =  mapped != cleaned;
  const bool control = _mask & XCB_MOD_MASK_CONTROL;
  const bool alt = _mask & XCB_MOD_MASK_1;

  w->on_key.fire(mapped, press);
  if (!remapped && !control && !alt && mapped != 0xfe03 && press)
    w->on_char.fire(mapped);
}

template<typename TResult>
hut::span<TResult> get_atom_values(xcb_connection_t *_con, xcb_window_t _win, xcb_atom_t _other, xcb_atom_t _type, size_t _count) {
  // https://stackoverflow.com/questions/37568142/what-event-is-used-for-maximizing-minimizing
  xcb_get_property_cookie_t cookie = xcb_get_property(_con, false, _win, _other, _type, 0, sizeof(TResult) / 4 * _count);
  xcb_generic_error_t *err = nullptr;
  xcb_get_property_reply_t *reply = xcb_get_property_reply(_con, cookie, &err);
  TResult *data = reinterpret_cast<TResult*>(xcb_get_property_value(reply));
  size_t size = xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);
  return {data, size};
}

std::string_view get_atom_name(xcb_connection_t *_con, xcb_atom_t _other) {
  xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(_con, _other);
  xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(_con, cookie, nullptr);
  const char *data = xcb_get_atom_name_name(reply);
  size_t size = static_cast<size_t>(xcb_get_atom_name_name_length(reply));
  return {data, size};
}

int display::dispatch() {
  if (windows_.empty())
    throw std::runtime_error("dispatch called without any window");

  bool loop = true;
  display::time_point now;

  while (loop) {
    xcb_generic_event_t *event = xcb_wait_for_event(connection_);
    do {
        now = display::clock::now();
        if (event != nullptr) {
          switch (event->response_type & ~0x80U) {
          case XCB_EXPOSE: {
            const auto *e = reinterpret_cast<xcb_expose_event_t *>(event);

            auto it = windows_.find(e->window);
            if (it != windows_.end()) {
              uvec4 r{e->x, e->y, e->x + e->width, e->y + e->height};
              window *w = it->second;
              if (!w->minimized_) {
                w->on_expose.fire(r);
                w->redraw(now);
              }
            }
          } break;

          case XCB_CONFIGURE_NOTIFY: {
            const auto *e = reinterpret_cast<xcb_configure_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              uvec2 s{e->width, e->height};
              if (s != w->size_) {
                w->dispatch_resize(s);
              }
            }
          } break;

          case XCB_KEY_PRESS: {
            auto *e = reinterpret_cast<xcb_key_press_event_t *>(event);

            int col;
            if (e->state & 0x3U)
              col = 1;  // shift || caps-lock
            else if (e->state & 0x80U)
              col = 4;  // alt-gr
            else
              col = 0;

            auto keysym = xcb_key_press_lookup_keysym(keysyms_, e, col);
            if (keysym >= 0xff80 && keysym <= 0xffb9 && keysym != XK_KP_Enter) {
              col = e->state & 0x10U ? 1 : 0;
              keysym = xcb_key_press_lookup_keysym(keysyms_, e, col);
            }

            auto it = windows_.find(e->event);
            if (it != windows_.end())
              dispatch_keysym(it->second, e->state, keysym, true);
          } break;

          case XCB_KEY_RELEASE: {
            auto *e = reinterpret_cast<xcb_key_release_event_t *>(event);

            int col;
            if (e->state & 0x80U)
              col = 4;  // alt-gr
            else if (e->state & 0x3U)
              col = 1;  // shift || caps-lock
            else
              col = 0;

            auto keysym = xcb_key_press_lookup_keysym(keysyms_, e, col);
            if (keysym >= 0xff80 && keysym <= 0xffb9 && keysym != XK_KP_Enter) {
              col = e->state & 0x10U ? 1 : 0;
              keysym = xcb_key_press_lookup_keysym(keysyms_, e, col);
            }
            if (keysym == 0) {
              keysym = xcb_key_press_lookup_keysym(keysyms_, e, 0);
            }

            auto it = windows_.find(e->event);
            if (it != windows_.end())
              dispatch_keysym(it->second, e->state, keysym, false);
          } break;

          case XCB_BUTTON_PRESS: {
            const auto *e = reinterpret_cast<xcb_button_press_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              uint8_t b = e->detail;
              auto c = vec2{e->event_x, e->event_y};
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
              w->on_mouse.fire(b, t, c);
            }
          } break;

          case XCB_BUTTON_RELEASE: {
            const auto *e = reinterpret_cast<xcb_button_release_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              uint8_t b = e->detail;
              if (b < 4 || b > 5) {  // ignore wheel events
                auto c = vec2{e->event_x, e->event_y};
                w->on_mouse.fire(b, mouse_event_type::MUP, c);
              }
            }
          } break;

          case XCB_MOTION_NOTIFY: {
            const auto *e = reinterpret_cast<xcb_motion_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              auto c = vec2{e->event_x, e->event_y};
              uint8_t b = e->detail;
              w->on_mouse.fire(b, mouse_event_type::MMOVE, c);
            }
          } break;

          case XCB_CLIENT_MESSAGE: {
            const auto *e = reinterpret_cast<xcb_client_message_event_t *>(event);

            auto it = windows_.find(e->window);
            if (it != windows_.end()) {
              if (e->data.data32[0] == atom_close_) {
                window *w = it->second;
                if (!w->on_close.fire())
                  w->close();
              }
            }
          } break;

          case XCB_FOCUS_IN: {
            const auto *e = reinterpret_cast<xcb_focus_in_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              w->on_focus.fire();
            }
          } break;

          case XCB_FOCUS_OUT: {
            const auto *e = reinterpret_cast<xcb_focus_out_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              w->on_blur.fire();
            }
          } break;

          case XCB_MAP_NOTIFY: {
            const auto *e = reinterpret_cast<xcb_map_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              w->on_resume.fire();
            }
          } break;

          case XCB_UNMAP_NOTIFY: {
            const auto *e = reinterpret_cast<xcb_unmap_notify_event_t *>(event);

            auto it = windows_.find(e->event);
            if (it != windows_.end()) {
              window *w = it->second;
              w->on_pause.fire();
            }
          } break;

          case XCB_PROPERTY_NOTIFY: {
            const auto *e = reinterpret_cast<xcb_property_notify_event_t *>(event);

            auto it = windows_.find(e->window);
            if (it != windows_.end()) {
              window *w = it->second;
              if (e->atom == atom_state_) {
                auto state_vals = get_atom_values<xcb_atom_t>(connection_, e->window, atom_state_, XCB_ATOM_ATOM, 32);
                bool is_minimized = std::find(state_vals.begin(), state_vals.end(), atom_hstate_) != state_vals.end();
                if (is_minimized && !w->minimized_) {
                  w->minimized_ = true;
                  w->on_pause.fire();
                }
                else if (!is_minimized && w->minimized_) {
                  w->minimized_ = false;
                  w->on_resume.fire();
                  w->invalidate(true);
                }
              }
            }
          } break;

          default:
            //std::cout << "ingored even of type: " << xcb_event_get_label(event->response_type) << std::endl;
            break;
          }
        }
        free(event);
        event = xcb_poll_for_event(connection_);
    } while (event != nullptr);
    process_posts(clock::now());
    if (windows_.empty())
      loop = false;
  }

  return EXIT_SUCCESS;
}
