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

#include "hut/window.hpp"

#include <cstring>

#include <iostream>

#include <unistd.h>

#include "hut/utils/profiling.hpp"

#include "hut/display.hpp"

using namespace hut;

void window::handle_xdg_configure(void *_data, xdg_surface *, u32 _serial) {
  auto *w = static_cast<window *>(_data);
  xdg_surface_ack_configure(w->window_, _serial);
}

void window::handle_toplevel_configure(void *_data, xdg_toplevel *, i32 _width, i32 _height, wl_array *_states) {
  auto *w = static_cast<window *>(_data);
  if (_width > 0 && _height > 0) {
    u16vec2 new_size{_width, _height};
    if (new_size != w->size_) {
      w->size_ = new_size;
      w->init_vulkan_surface();
      HUT_PROFILE_EVENT(w, on_resize, new_size, w->scale_);
    }
  }
  std::span<xdg_toplevel_state> states{reinterpret_cast<xdg_toplevel_state *>(_states->data),
                                       static_cast<std::size_t>(_states->size / sizeof(xdg_toplevel_state))};
  bool minimized = std::find(states.begin(), states.end(), XDG_TOPLEVEL_STATE_ACTIVATED) == states.end();
  // FIXME: This doesn't really represents minimized state, loosing focus also lose this state
  if (!minimized && w->minimized_) {
    w->minimized_ = false;
    HUT_PROFILE_EVENT(w, on_resume);
  } else if (minimized && !w->minimized_) {
    w->minimized_ = true;
    HUT_PROFILE_EVENT(w, on_pause);
  }
}

void window::handle_toplevel_close(void *_data, xdg_toplevel *) {
  auto *w = static_cast<window *>(_data);
  if (!HUT_PROFILE_EVENT(w, on_close))
    w->close();
}

void window::trigger_scale() {
  u32   scale  = numax_v<u32>;
  auto &scales = display_.outputs_scale_;
  if (scales.empty()) {
    scale = 1;
  } else {
    for (auto output : overlapping_outputs_) {
      assert(scales.contains(output));
      scale = std::min(scale, scales[output]);
    }
  }
  if (scale != scale_ && scale != numax_v<u32>) {
    scale_ = scale;
    display_.cursor(current_cursor_type_, scale);
    wl_surface_set_buffer_scale(wayland_surface_, int(scale));
    init_vulkan_surface();
    HUT_PROFILE_EVENT(this, on_resize, size_, scale);
  }
}

void window::surface_enter(void *_data, struct wl_surface *_surface, struct wl_output *_output) {
#if defined(HUT_ENABLE_VALIDATION_DEBUG) && 0
  std::cout << "[hut] surface_enter " << _surface << ", " << _output << std::endl;
#endif
  auto *w      = static_cast<window *>(_data);
  auto  result = w->overlapping_outputs_.insert(_output);
  if (result.second)
    w->trigger_scale();
}
void window::surface_leave(void *_data, struct wl_surface *_surface, struct wl_output *_output) {
#if defined(HUT_ENABLE_VALIDATION_DEBUG) && 0
  std::cout << "[hut] surface_leave " << _surface << ", " << _output << std::endl;
#endif
  auto *w = static_cast<window *>(_data);
  if (w->overlapping_outputs_.erase(_output))
    w->trigger_scale();
}

window::window(display &_display, const window_params &_init_params)
    : render_target(_display)
    , display_(_display)
    , params_(_init_params)
    , size_(_init_params.size_) {
  HUT_PROFILE_FUN(PWAYLAND)
  static const wl_surface_listener   wl_surface_listeners   = {surface_enter, surface_leave};
  static const xdg_surface_listener  xdg_surface_listeners  = {handle_xdg_configure};
  static const xdg_toplevel_listener xdg_toplevel_listeners = {handle_toplevel_configure, handle_toplevel_close};

  assert(_display.compositor_);
  wayland_surface_ = wl_compositor_create_surface(_display.compositor_);
  if (!wayland_surface_)
    throw std::runtime_error("couldn't create wayland surface for new window");
  wl_surface_add_listener(wayland_surface_, &wl_surface_listeners, this);

  VkWaylandSurfaceCreateInfoKHR info = {};
  info.sType                         = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
  info.display                       = _display.display_;
  info.surface                       = wayland_surface_;

  if (_init_params.flags_ & window_params::FFULLSCREEN)
    fullscreen();

  auto     func = _display.get_proc<PFN_vkCreateWaylandSurfaceKHR>("vkCreateWaylandSurfaceKHR");
  VkResult vkr;
  if ((vkr = func(_display.instance_, &info, nullptr, &surface_)) != VK_SUCCESS)
    throw std::runtime_error(sstream("couldn't create vulkan dummy surface, code: ") << vkr);

  _display.windows_.emplace(wayland_surface_, this);

  assert(_display.xdg_wm_base_);
  window_ = xdg_wm_base_get_xdg_surface(_display.xdg_wm_base_, wayland_surface_);
  xdg_surface_add_listener(window_, &xdg_surface_listeners, this);

  toplevel_ = xdg_surface_get_toplevel(window_);
  xdg_toplevel_add_listener(toplevel_, &xdg_toplevel_listeners, this);

  if (_init_params.min_size_ != uvec2{0, 0})
    xdg_toplevel_set_min_size(toplevel_, int(_init_params.min_size_.x), int(_init_params.min_size_.y));
  if (_init_params.max_size_ != uvec2{0, 0})
    xdg_toplevel_set_max_size(toplevel_, int(_init_params.max_size_.x), int(_init_params.max_size_.y));

  wl_surface_commit(wayland_surface_);
  init_vulkan_surface();
}

window::~window() {
  HUT_PROFILE_FUN(PWAYLAND)
  close();
}

void window::close() {
  if (window_ != nullptr) {
    HUT_PROFILE_FUN(PWAYLAND)
    display_.windows_.erase(wayland_surface_);
    display_.pointer_current_ = {nullptr, nullptr};

    destroy_vulkan();
    xdg_toplevel_destroy(toplevel_);
    xdg_surface_destroy(window_);
    wl_surface_destroy(wayland_surface_);
    window_ = nullptr;

    display_.post_empty_event();
  }
}

void window::title(std::u8string_view _title) {
  HUT_PROFILE_FUN(PWAYLAND)
  assert(_title.size() < 1024);
  char buffer[1024];
  auto min = std::min(_title.size(), sizeof(buffer) - 1);
  memcpy(buffer, _title.data(), min);
  buffer[min] = 0;
  xdg_toplevel_set_title(toplevel_, buffer);
}

void window::pause() {
  xdg_toplevel_set_minimized(toplevel_);
}

void window::maximize(bool _set) {
  if (_set) {
    xdg_toplevel_set_maximized(toplevel_);
  } else {
    xdg_toplevel_unset_maximized(toplevel_);
  }
}

void window::fullscreen(bool _set) {
  if (_set) {
    xdg_toplevel_set_fullscreen(toplevel_, nullptr);
  } else {
    xdg_toplevel_unset_fullscreen(toplevel_);
  }
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
  display_.cursor(_c, scale_);
}

void window::clipboard_data_source_handle_send(void *_data, wl_data_source *_source, const char *_mime, i32 _fd) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "clipboard_data_source_handle_send " << _source << std::endl;
#endif

  auto *w      = (window *)_data;
  auto  format = mime_type_format(_mime);
  if (format) {
    auto &ctx = w->clipboard_sender_context_;
    assert(ctx.selection_ == _source);
    auto &writers  = ctx.writers_;
    auto  writerit = writers.emplace(writers.end());
    assert(writers.size() < 8);
    writerit->sender_.open(_fd);
    writerit->thread_ = std::thread([w, cb = ctx.callback_, writerit, f = format.value()]() {
      cb(f, writerit->sender_);
      writerit->sender_.close();
      w->display_.post([w, writerit](auto) { w->clipboard_sender_context_.writers_.erase(writerit); });
    });
  }
}

void window::clipboard_data_source_handle_cancelled(void *_data, wl_data_source *_source) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "clipboard_data_source_handle_cancelled" << std::endl;
#endif

  auto *w = (window *)_data;
  w->clipboard_sender_context_.clear();
}

void window::clipboard_data_source_handle_target(void *_data, wl_data_source *_source, const char *_mime) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "clipboard_data_source_handle_target" << std::endl;
#endif
}
void window::clipboard_data_source_handle_dnd_drop_performed(void *_data, wl_data_source *_source) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "clipboard_data_source_handle_dnd_drop_performed" << std::endl;
#endif
}
void window::clipboard_data_source_handle_dnd_finished(void *_data, wl_data_source *_source) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "clipboard_data_source_handle_dnd_finished" << std::endl;
#endif
}
void window::clipboard_data_source_handle_action(void *_data, wl_data_source *_source, u32 _action) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "clipboard_data_source_handle_action" << std::endl;
#endif
}

void window::clipboard_offer(clipboard_formats _supported_formats, send_clipboard_data &&_callback) {
  static const wl_data_source_listener wl_data_source_listeners
      = {clipboard_data_source_handle_target,       clipboard_data_source_handle_send,
         clipboard_data_source_handle_cancelled,    clipboard_data_source_handle_dnd_drop_performed,
         clipboard_data_source_handle_dnd_finished, clipboard_data_source_handle_action};

  HUT_PROFILE_FUN(PWAYLAND)
  if (!display_.data_device_manager_ || !display_.data_device_)
    return;

  if (!_supported_formats)
    return;

  auto *source = wl_data_device_manager_create_data_source(display_.data_device_manager_);
  clipboard_sender_context_.reset(source, std::move(_callback));

#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "window::clipboard_offer " << source << std::endl;
#endif

  wl_data_source_add_listener(source, &wl_data_source_listeners, this);
  for (auto mime : _supported_formats) {
    wl_data_source_offer(source, format_mime_type(mime));
  }

  wl_data_device_set_selection(display_.data_device_, source, display_.last_serial_);
}

bool window::clipboard_receive(clipboard_formats _supported_formats, receive_clipboard_data &&_callback) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "window::clipboard_receive" << std::endl;
#endif

  HUT_PROFILE_FUN(PWAYLAND)
  if (display_.last_offer_from_clipboard_ == nullptr)
    return false;

  for (auto mime : _supported_formats) {
    auto *offer  = display_.last_offer_from_clipboard_;
    auto &params = display_.offer_params_[offer];
    if (params.formats_ & mime) {
      int fds[2];
      if (pipe(fds) != 0)
        return false;
      wl_data_offer_receive(offer, format_mime_type(mime), fds[1]);
      ::close(fds[1]);

      auto reader = async_readers_.emplace(async_readers_.end());
      assert(async_readers_.size() < 8);
      reader->receiver_.open(fds[0]);
      reader->thread_ = std::thread([this, cb = std::move(_callback), mime, reader]() {
        cb(mime, reader->receiver_);
        this->display_.post([this, reader](auto) { this->async_readers_.erase(reader); });
      });
      return true;
    }
  }

  return false;
}

void window::drag_data_source_handle_send(void *_data, wl_data_source *_source, const char *_mime, i32 _fd) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "drag_data_source_handle_send " << _source << ", " << _mime << std::endl;
#endif

  auto *w      = (window *)_data;
  auto  format = mime_type_format(_mime);
  if (format) {
    auto it = w->dragndrop_async_writers_.find(_source);
    assert(it != w->dragndrop_async_writers_.end());
    auto writer = it->second;
    writer->sender_.open(_fd);
    writer->thread_ = std::thread([f = format.value(), writer]() {
      writer->callback_(writer->last_drag_action_handled_, f, writer->sender_);
      writer->sender_.close();
    });
  }
}

void window::drag_data_source_handle_target(void *_data, wl_data_source *_source, const char *_mime) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  //std::cout << "drag_data_source_handle_target " << _source << ", " << (_mime ? _mime : "<nullptr>") << std::endl;
#endif

  auto *w = (window *)_data;
  if (_mime == nullptr) {
    auto it = w->dragndrop_async_writers_.find(_source);
    assert(it != w->dragndrop_async_writers_.end());
    it->second->last_drag_action_handled_ = DNDNONE;
  }
}

void window::drag_data_source_handle_action(void *_data, wl_data_source *_source, u32 _action) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "drag_data_source_handle_action " << _source << ", " << _action << std::endl;
#endif

  auto *w  = (window *)_data;
  auto  it = w->dragndrop_async_writers_.find(_source);
  assert(it != w->dragndrop_async_writers_.end());
  if (_action == WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY)
    it->second->last_drag_action_handled_ = DNDCOPY;
  else if (_action == WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE)
    it->second->last_drag_action_handled_ = DNDMOVE;
}

void window::drag_data_source_handle_cancelled(void *_data, wl_data_source *_source) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "drag_data_source_handle_cancelled " << _source << std::endl;
#endif

  auto *w  = (window *)_data;
  auto  it = w->dragndrop_async_writers_.find(_source);
  assert(it != w->dragndrop_async_writers_.end());
  wl_data_source_destroy(_source);
  w->dragndrop_async_writers_.erase(it);
}

void window::drag_data_source_handle_dnd_drop_performed(void *_data, wl_data_source *_source) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "drag_data_source_handle_dnd_drop_performed " << _source << std::endl;
#endif
}

void window::drag_data_source_handle_dnd_finished(void *_data, wl_data_source *_source) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "drag_data_source_handle_dnd_finished " << _source << std::endl;
#endif

  auto *w  = (window *)_data;
  auto  it = w->dragndrop_async_writers_.find(_source);
  assert(it != w->dragndrop_async_writers_.end());
  wl_data_source_destroy(_source);
  w->dragndrop_async_writers_.erase(it);
}

void window::dragndrop_start(dragndrop_actions _supported_actions, clipboard_formats _supported_formats,
                             send_dragndrop_data &&_callback) {
  static const wl_data_source_listener wl_data_source_listeners
      = {drag_data_source_handle_target,       drag_data_source_handle_send,
         drag_data_source_handle_cancelled,    drag_data_source_handle_dnd_drop_performed,
         drag_data_source_handle_dnd_finished, drag_data_source_handle_action};

  HUT_PROFILE_FUN(PWAYLAND)
  if (!display_.data_device_manager_ || !display_.data_device_)
    return;
  if (!_supported_formats)
    return;

  auto *source    = wl_data_device_manager_create_data_source(display_.data_device_manager_);
  auto  data      = std::make_shared<dragndrop_async_writer>();
  data->callback_ = std::move(_callback);
  assert(dragndrop_async_writers_.find(source) == dragndrop_async_writers_.end());
  dragndrop_async_writers_[source] = data;
  assert(dragndrop_async_writers_.size() < 8);

  wl_data_source_add_listener(source, &wl_data_source_listeners, this);
  for (auto mime : _supported_formats) {
    wl_data_source_offer(source, format_mime_type(mime));
  }
  u32 wl_actions = 0;
  for (auto action : _supported_actions) {
    switch (action) {
      default: break;
      case DNDCOPY: wl_actions |= WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY; break;
      case DNDMOVE: wl_actions |= WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE; break;
    }
  }
  wl_data_source_set_actions(source, wl_actions);
  display_.cursor(CGRABBING, scale_);

  assert(display_.last_mouse_click_serial_ != 0);
  wl_data_device_start_drag(display_.data_device_, source, wayland_surface_, nullptr,
                            display_.last_mouse_click_serial_);
}

void window::interactive_resize(edge _edge) {
  assert(display_.last_mouse_click_serial_ != 0);
  assert(display_.seat_);
  xdg_toplevel_resize(toplevel_, display_.seat_, display_.last_mouse_click_serial_, _edge.raw());
}

void window::interactive_move() {
  assert(display_.last_mouse_click_serial_ != 0);
  assert(display_.seat_);
  xdg_toplevel_move(toplevel_, display_.seat_, display_.last_mouse_click_serial_);
}
