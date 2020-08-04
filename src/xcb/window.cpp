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

#include <unordered_set>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

window::window(display &_display) : display_(_display), size_(800, 600), parent_(_display.screen_->root) {
  window_ = xcb_generate_id(_display.connection_);

  uint32_t mask = XCB_CW_EVENT_MASK;
  uint32_t values[1] = {XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS
                        | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_KEYMAP_STATE
                        | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE
                        | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY};

  xcb_create_window(_display.connection_, XCB_COPY_FROM_PARENT, window_, parent_, 0, 0, 800, 600, 0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, _display.screen_->root_visual, mask, values);

  xcb_change_property(_display.connection_, XCB_PROP_MODE_REPLACE, window_, _display.atom_wm_, 4, 32, 1,
                      &_display.atom_close_);

  VkXcbSurfaceCreateInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
  info.connection = _display.connection_;
  info.window = window_;

  auto func = display_.get_proc<PFN_vkCreateXcbSurfaceKHR>("vkCreateXcbSurfaceKHR");
  VkResult result;
  if ((result = func(_display.instance_, &info, nullptr, &surface_)) != VK_SUCCESS)
    throw std::runtime_error(sstream("failed to create window surface, code: ") << result);

  _display.windows_.emplace(window_, this);

  init_vulkan_surface();

  // borderless window
  struct {
    uint32_t   flags = 2;
    uint32_t   functions = 0;
    uint32_t   decorations = 0;
    int32_t    input_mode = 0;
    uint32_t   status = 0;
  } hints;
  xcb_change_property(_display.connection_, XCB_PROP_MODE_REPLACE, window_, _display.atom_window_hints_,
                      XCB_ATOM_ATOM, 32, 5, &hints);

  xcb_map_window(_display.connection_, window_);
  xcb_flush(_display.connection_);
}

window::~window() {
  close();
}

void window::close() {
  if (window_ != 0) {
    display_.windows_.erase(window_);

    destroy_vulkan();
    xcb_unmap_window(display_.connection_, window_);
    xcb_destroy_window(display_.connection_, window_);
    xcb_flush(display_.connection_);
    window_ = 0;

    display_.post_empty_event();
  }
}

void window::pause() {
  xcb_client_message_event_t event;
  event.response_type = XCB_CLIENT_MESSAGE;
  event.format = 32;
  event.window = window_;
  event.type = display_.atom_change_state_;
  event.data.data32[0] = 3; // ICONIC

  xcb_send_event(display_.connection_, 0, display_.screen_->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (const char *)&event);
  xcb_flush(display_.connection_);
}

void window::maximize(bool _set) {
  xcb_client_message_event_t event;
  event.response_type = XCB_CLIENT_MESSAGE;
  event.format = 32;
  event.window = window_;
  event.type = display_.atom_state_;
  event.data.data32[0] = _set ? 1 : 0; // ADD or REMOVE
  event.data.data32[1] = display_.atom_maximizev_;
  event.data.data32[2] = display_.atom_maximizeh_;
  event.data.data32[3] = 1; // Source == app
  event.data.data32[4] = 0;

  xcb_send_event(display_.connection_, 0, display_.screen_->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (const char *)&event);
  xcb_flush(display_.connection_);
}

void window::title(const std::string &_title) {
  xcb_change_property(display_.connection_, XCB_PROP_MODE_REPLACE, window_, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                      (uint32_t)_title.size(), _title.data());
  xcb_change_property(display_.connection_, XCB_PROP_MODE_REPLACE, window_, XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8,
                      (uint32_t)_title.size(), _title.data());
}

void window::invalidate(const uvec4 &_coords, bool _redraw) {
  struct cmd_t {
    uvec4 coords_;
    bool redraw_;
    bool operator==(const cmd_t&) const = default;
  };
  struct cmd_hasher {
    std::size_t operator()(const cmd_t& _node) const {
      std::size_t seed = 0;
      hash_combine(seed, _node.coords_, _node.redraw_);
      return seed;
    }
  };
  static std::mutex mutex;
  static std::unordered_set<cmd_t, cmd_hasher> posted_this_frame;

  if (_redraw) {
    for (auto &d : dirty_)
      d = true;
  }

  cmd_t thiz {_coords, _redraw};
  {
    std::scoped_lock sl(mutex);
    if (!posted_this_frame.emplace(thiz).second)
      return;
  }

  xcb_expose_event_t event = {};
  event.window = window_;
  event.x = (uint16_t)_coords.x;
  event.y = (uint16_t)_coords.y;
  event.width = (uint16_t)(_coords.z - _coords.x);
  event.height = (uint16_t)(_coords.w - _coords.y);
  event.count = 1;
  event.response_type = XCB_EXPOSE;
  uint32_t mask = XCB_EVENT_MASK_EXPOSURE;
  xcb_send_event(display_.connection_, 0, window_, mask, (const char *)&event);
  xcb_flush(display_.connection_);

  display_.post([&](display::time_point) {
    std::scoped_lock sl(mutex);
    posted_this_frame.clear();
  });
}

void window::set_cursor(cursor_type _c) {
  xcb_cursor_t cursor = xcb_cursor_load_cursor(display_.cursor_context_, cursor_css_name(_c));
  if (cursor != XCB_CURSOR_NONE)
    xcb_change_window_attributes(display_.connection_, window_, XCB_CW_CURSOR, &cursor);
}
