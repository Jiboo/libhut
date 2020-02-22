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

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

void hut::handle_xdg_configure(void *_data, xdg_surface *, uint32_t _serial) {
  auto *w = static_cast<window*>(_data);
  xdg_surface_ack_configure(w->window_, _serial);
}

void hut::handle_toplevel_configure(void *_data, xdg_toplevel *, int32_t _width, int32_t _height, wl_array *) {
  if (_width == 0 || _height == 0)
    return;

  auto *w = static_cast<window*>(_data);
  uvec2 new_size{_width, _height};
  if (new_size != w->size_)
    w->dispatch_resize(new_size);
}

void hut::handle_toplevel_close(void *_data, xdg_toplevel *) {
  auto *w = static_cast<window*>(_data);
  if (!w->on_close.fire())
    w->close();
}

window::window(display &_display) : display_(_display), size_(800, 600) {
  static const xdg_surface_listener xdg_surface_listener = {
      handle_xdg_configure,
  };
  static const xdg_toplevel_listener xdg_toplevel_listener = {
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
  xdg_surface_add_listener(window_, &xdg_surface_listener, this);

  toplevel_ = xdg_surface_get_toplevel(window_);
  xdg_toplevel_add_listener(toplevel_, &xdg_toplevel_listener, this);

  init_vulkan_surface();
  wl_surface_commit(wayland_surface_);
}

window::~window() {
  close();
}

void window::close() {
  if (window_ != nullptr) {
    display_.windows_.erase(wayland_surface_);

    destroy_vulkan();
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

void window::invalidate(const uvec4 &, bool _redraw) {
  if (_redraw) {
    for (size_t i = 0; i < dirty_.size(); i++)
      dirty_[i] = true;
  }
  invalidated_ = true;
  display_.post_empty_event();
}
