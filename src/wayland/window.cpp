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

#include <unistd.h>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

void hut::handle_xdg_configure(void *_data, xdg_surface *, uint32_t _serial) {
  auto *w = static_cast<window*>(_data);
  xdg_surface_ack_configure(w->window_, _serial);
}

void hut::handle_toplevel_configure(void *_data, xdg_toplevel *, int32_t _width, int32_t _height, wl_array *_states) {
  auto *w = static_cast<window*>(_data);
  if (_width > 0 && _height > 0) {
    uvec2 new_size{_width, _height};
    if (new_size != w->size_) {
      w->size_ = new_size;
      w->init_vulkan_surface();
      HUT_PROFILE_EVENT(w, on_resize, new_size);
    }
  }
  span<xdg_toplevel_state> states {
      reinterpret_cast<xdg_toplevel_state*>(_states->data),
      static_cast<std::size_t>(_states->size / sizeof(xdg_toplevel_state))
  };
  bool minimized = std::find(states.begin(), states.end(), XDG_TOPLEVEL_STATE_ACTIVATED) == states.end();
  // FIXME: This doesn't really represent minimized state, loosing focus also lose this state
  if (!minimized && w->minimized_) {
    w->minimized_ = false;
    HUT_PROFILE_EVENT(w, on_resume);
  }
  else if (minimized && !w->minimized_) {
    w->minimized_ = true;
    HUT_PROFILE_EVENT(w, on_pause);
  }
}

void hut::handle_toplevel_close(void *_data, xdg_toplevel *) {
  auto *w = static_cast<window*>(_data);
  if (!HUT_PROFILE_EVENT(w, on_close))
    w->close();
}

void hut::handle_toplevel_decoration_configure(void *_data, zxdg_toplevel_decoration_v1 *_deco, uint32_t _mode) {
  auto *w = static_cast<window*>(_data);
  w->has_system_decorations_ = _mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
}

window::window(display &_display, const window_params &_init_params)
  : display_(_display), size_(bbox_size(_init_params.position_)) {
  static const xdg_surface_listener xdg_surface_listeners = {
    handle_xdg_configure,
  };
  static const xdg_toplevel_listener xdg_toplevel_listeners = {
    handle_toplevel_configure, handle_toplevel_close
  };

  wayland_surface_ = wl_compositor_create_surface(_display.compositor_);
  if (!wayland_surface_)
    throw std::runtime_error("couldn't create wayland surface for new window");

  VkWaylandSurfaceCreateInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
  info.display = _display.display_;
  info.surface = wayland_surface_;

  auto func = _display.get_proc<PFN_vkCreateWaylandSurfaceKHR>("vkCreateWaylandSurfaceKHR");
  VkResult vkr;
  if ((vkr = func(_display.instance_, &info, nullptr, &surface_)) != VK_SUCCESS)
    throw std::runtime_error(sstream("couldn't create vulkan dummy surface, code: ") << vkr);

  _display.windows_.emplace(wayland_surface_, this);

  window_ = xdg_wm_base_get_xdg_surface(_display.xdg_wm_base_, wayland_surface_);
  xdg_surface_add_listener(window_, &xdg_surface_listeners, this);

  toplevel_ = xdg_surface_get_toplevel(window_);
  xdg_toplevel_add_listener(toplevel_, &xdg_toplevel_listeners, this);

  has_system_decorations_ = false;
  if (_init_params.flags_ & window_params::SYSTEM_DECORATIONS && display_.decoration_manager_) {
    static const zxdg_toplevel_decoration_v1_listener zxdg_toplevel_decoration_v1_listeners = {
        handle_toplevel_decoration_configure,
    };
    decoration_ = zxdg_decoration_manager_v1_get_toplevel_decoration(display_.decoration_manager_, toplevel_);
    zxdg_toplevel_decoration_v1_add_listener(decoration_, &zxdg_toplevel_decoration_v1_listeners, this);
    zxdg_toplevel_decoration_v1_set_mode(decoration_, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    has_system_decorations_ = true;
  }
  if (_init_params.min_size_ != uvec2{0, 0})
    xdg_toplevel_set_min_size(toplevel_, _init_params.min_size_.x, _init_params.min_size_.y);
  if (_init_params.max_size_ != uvec2{0, 0})
    xdg_toplevel_set_max_size(toplevel_, _init_params.max_size_.x, _init_params.max_size_.y);

  wl_surface_commit(wayland_surface_);
  init_vulkan_surface();
}

window::~window() {
  close();
}

void window::close() {
  if (window_ != nullptr) {
    display_.windows_.erase(wayland_surface_);
    display_.pointer_current_ = {nullptr, nullptr};

    destroy_vulkan();
    if (decoration_) zxdg_toplevel_decoration_v1_destroy(decoration_);
    xdg_toplevel_destroy(toplevel_);
    xdg_surface_destroy(window_);
    wl_surface_destroy(wayland_surface_);
    window_ = nullptr;

    display_.post_empty_event();
  }
}

void window::title(const std::string &_title) {
  xdg_toplevel_set_title(toplevel_, _title.c_str());
}

void window::pause() {
  xdg_toplevel_set_minimized(toplevel_);
}

void window::maximize(bool _set) {
  if (_set)
    xdg_toplevel_set_maximized(toplevel_);
  else
    xdg_toplevel_unset_maximized(toplevel_);
}

void window::fullscreen(bool _set) {
  if (_set)
    xdg_toplevel_set_fullscreen(toplevel_, nullptr);
  else
    xdg_toplevel_unset_fullscreen(toplevel_);
}

void window::invalidate(const uvec4 &, bool _redraw) {
  if (_redraw) {
    for (auto &d : dirty_)
      d = true;
  }
  invalidated_ = true;
  display_.post_empty_event();
}

void window::cursor(cursor_type _c) {
  current_cursor_type_ = _c;
  display_.cursor(_c);
}

void hut::data_source_handle_send(void *_data, wl_data_source *_source, const char *_mime, int32_t _fd) {
  auto *w = (window*)_data;
  auto format = mime_type_format(_mime);
  if (!format) {
    if (strcmp("text/plain;charset=utf-8", _mime) == 0) format = FTEXT_PLAIN;
    else if (strcmp("text/plain;charset=unicode", _mime) == 0) format = FTEXT_PLAIN;
    else if (strcmp("STRING", _mime) == 0) format = FTEXT_PLAIN;
    else if (strcmp("TEXT", _mime) == 0) format = FTEXT_PLAIN;
    else if (strcmp("UTF8_STRING", _mime) == 0) format = FTEXT_PLAIN;
  }
  if (format) {
    window::clipboard_sender sender{_fd};
    w->current_clipboard_sender_(*format, sender);
  }
  ::close(_fd);
}

size_t window::clipboard_sender::write(span<uint8_t> _data) {
  return ::write(fd_, _data.data(), _data.size_bytes());
}
size_t window::clipboard_receiver::read(span<uint8_t> _data) {
  return ::read(fd_, _data.data(), _data.size_bytes());
}

void hut::data_source_handle_cancelled(void *_data, wl_data_source *_source) {
  wl_data_source_destroy(_source);
  auto *w = (window*)_data;
  w->current_selection_ = nullptr;
}

void data_source_handle_target(void *data, struct wl_data_source *wl_data_source, const char *mime_type) {}

void window::clipboard_offer(clipboard_formats _supported_formats, const send_clipboard_data &_callback) {
  static const wl_data_source_listener wl_data_source_listeners = {
    data_source_handle_target,
    data_source_handle_send,
    data_source_handle_cancelled,
  };

  if (!display_.data_device_manager_ || !display_.data_device_)
    return;

  if (current_selection_) {
    wl_data_source_destroy(current_selection_);
    current_selection_ = nullptr;
  }

  if (!_supported_formats)
    return;

  current_clipboard_sender_ = _callback;
  current_selection_ = wl_data_device_manager_create_data_source(display_.data_device_manager_);
  wl_data_source_add_listener(current_selection_, &wl_data_source_listeners, this);
  for (auto mime : _supported_formats) {
    wl_data_source_offer(current_selection_, format_mime_type(mime));
    switch(mime) {
      default: break;
      case FTEXT_URI_LIST:
      case FTEXT_HTML:
      case FTEXT_PLAIN:
        wl_data_source_offer(current_selection_, "text/plain;charset=utf-8");
        wl_data_source_offer(current_selection_, "text/plain;charset=unicode");
        wl_data_source_offer(current_selection_, "STRING");
        wl_data_source_offer(current_selection_, "TEXT");
        wl_data_source_offer(current_selection_, "UTF8_STRING");
        break;
    }
  }

  wl_data_device_set_selection(display_.data_device_, current_selection_, display_.last_serial_);
}

bool window::clipboard_receive(clipboard_formats _supported_formats, const receive_clipboard_data &_callback) {
  if (display_.current_data_offer_ == nullptr)
    return false;

  for (auto mime : _supported_formats) {
    if (display_.current_data_offer_formats_ & mime) {
      int fds[2];
      if (pipe(fds) != 0)
        return false;
      wl_data_offer_receive(display_.current_data_offer_, format_mime_type(mime), fds[1]);
      ::close(fds[1]);

      wl_display_roundtrip(display_.display_);

      clipboard_receiver receiver{fds[0]};
      _callback(mime, receiver);
      ::close(fds[0]);
      return true;
    }
  }

  return false;
}

void window::interactive_resize(edge _edge) {
  // TODO Assert called from on_mouse (move) event
  xdg_toplevel_resize(toplevel_, display_.seat_, display_.last_serial_, _edge.active_);
}

void window::interactive_move() {
  // TODO Assert called from on_mouse (move) event
  xdg_toplevel_move(toplevel_, display_.seat_, display_.last_serial_);
}
