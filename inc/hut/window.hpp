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

#pragma once

#include <chrono>
#include <string>

#include <wayland-client.h>

#include "hut/utils/event.hpp"

#include "hut/display.hpp"
#include "hut/render_target.hpp"

namespace hut {

enum side {
  TOP,
  BOTTOM,
  LEFT,
  RIGHT,
  LAST_SIDE = RIGHT,
};
using edge = flagged<side, LAST_SIDE>;
cursor_type edge_cursor(edge);

enum touch_event_type { TDOWN, TUP, TMOVE };
const char          *touch_event_name(touch_event_type);
inline std::ostream &operator<<(std::ostream &_os, touch_event_type _c) {
  return _os << touch_event_name(_c);
}

enum mouse_event_type { MDOWN, MUP, MMOVE, MWHEEL_UP, MWHEEL_DOWN };
const char          *mouse_event_name(mouse_event_type);
inline std::ostream &operator<<(std::ostream &_os, mouse_event_type _c) {
  return _os << mouse_event_name(_c);
}

class display;

struct window_params {
  enum flag {
    FDEPTH         = render_target_params::flag::FDEPTH,
    FMULTISAMPLING = render_target_params::flag::FMULTISAMPLING,
    FVSYNC,
    FTRANSPARENT,
    FFULLSCREEN,
    FLAG_LAST_VALUE = FFULLSCREEN,
  };
  using flags = flagged<flag, flag::FLAG_LAST_VALUE>;

  flags flags_{FVSYNC};
  uvec2 size_ = {800, 600}, min_size_ = {0, 0}, max_size_ = {0, 0};
};

class window : public render_target {
  friend class display;

 public:
  event<>                  on_pause, on_resume, on_focus, on_blur, on_close;
  event<uvec4>             on_expose;
  event<uvec2>             on_resize;
  event<VkCommandBuffer>   on_draw;
  event<display::duration> on_frame;

  event<u8 /*finger*/, touch_event_type, vec2 /*pos*/> on_touch;
  event<u8 /*button*/, mouse_event_type, vec2 /*pos*/> on_mouse;

  event<u32> on_time;

  event<modifiers /*mods*/>             on_kmods;
  event<keycode, keysym, bool /*down*/> on_key;
  event<char32_t /*utf32_char*/>        on_char;

  window() = delete;

  window(const window &) = delete;
  window &operator=(const window &) = delete;

  window(window &&) noexcept = delete;
  window &operator=(window &&) noexcept = delete;

  explicit window(display &_display, const window_params &_init_params = {});
  ~window();

  void close();
  void pause();

  void invalidate(const uvec4 &, bool _redraw);
  void invalidate(bool _redraw);

  void  maximize(bool _set = true);
  void  fullscreen(bool _set = true);
  uvec2 size() { return size_; }
  void  interactive_resize(edge);
  void  interactive_move();

  void title(std::u8string_view);
  void cursor(cursor_type _c);
  void clear_color(const vec4 &_color) {
    render_target_params_.clear_color_ = {_color.r, _color.g, _color.b, _color.a};
    invalidate(true);
  }

  using send_clipboard_data = std::function<void(clipboard_format /*_selected_format*/, clipboard_sender &)>;
  void clipboard_offer(clipboard_formats _supported_formats, send_clipboard_data &&_callback);

  using receive_clipboard_data = std::function<void(clipboard_format /*selected_format*/, clipboard_receiver &)>;
  bool clipboard_receive(clipboard_formats _supported_formats, receive_clipboard_data &&_callback);

  void dragndrop_target(const std::shared_ptr<drop_target_interface> &_received) { drop_target_interface_ = _received; }

  using send_dragndrop_data
      = std::function<void(dragndrop_action /*_action*/, clipboard_format /*_selected_format*/, clipboard_sender &)>;
  void dragndrop_start(dragndrop_actions _supported_actions, clipboard_formats _supported_formats,
                       send_dragndrop_data &&_callback);

 protected:
  display      &display_;
  window_params params_;

  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  VkPresentModeKHR   present_mode_;
  VkSurfaceFormatKHR surface_format_;

  VkExtent2D                   swapchain_extents_;
  VkSwapchainKHR               swapchain_ = VK_NULL_HANDLE;
  std::vector<VkImage>         swapchain_images_;
  std::vector<VkImageView>     swapchain_imageviews_;
  std::vector<VkCommandBuffer> primary_cbs_;
  std::vector<VkCommandBuffer> cbs_;
  std::vector<u8>              dirty_;

  VkSemaphore sem_available_ = VK_NULL_HANDLE;
  VkSemaphore sem_rendered_  = VK_NULL_HANDLE;

  uvec2               size_, pos_;
  display::time_point last_frame_ = display::clock::now();
  bool                minimized_  = false;

  std::shared_ptr<drop_target_interface> drop_target_interface_;

  void init_vulkan_surface();
  void destroy_vulkan();
  void redraw(display::time_point);

 protected:
  static void handle_xdg_configure(void *, xdg_surface *, u32);
  static void handle_toplevel_configure(void *, xdg_toplevel *, i32, i32, wl_array *);
  static void handle_toplevel_close(void *, xdg_toplevel *);
  static void clipboard_data_source_handle_target(void *, wl_data_source *, const char *);
  static void clipboard_data_source_handle_send(void *, wl_data_source *, const char *, i32);
  static void clipboard_data_source_handle_cancelled(void *, wl_data_source *);
  static void clipboard_data_source_handle_dnd_drop_performed(void *, wl_data_source *);
  static void clipboard_data_source_handle_dnd_finished(void *, wl_data_source *);
  static void clipboard_data_source_handle_action(void *, wl_data_source *, u32);
  static void drag_data_source_handle_target(void *, wl_data_source *, const char *);
  static void drag_data_source_handle_send(void *, wl_data_source *, const char *, i32);
  static void drag_data_source_handle_cancelled(void *, wl_data_source *);
  static void drag_data_source_handle_dnd_drop_performed(void *, wl_data_source *);
  static void drag_data_source_handle_dnd_finished(void *, wl_data_source *);
  static void drag_data_source_handle_action(void *, wl_data_source *, u32);

  wl_surface   *wayland_surface_;
  xdg_surface  *window_;
  xdg_toplevel *toplevel_;

  std::atomic_bool invalidated_         = true;
  vec2             mouse_lastmove_      = {0, 0};
  cursor_type      current_cursor_type_ = CDEFAULT;

  struct dragndrop_async_writer {
    send_dragndrop_data callback_;
    std::thread         thread_;
    clipboard_sender    sender_;
    dragndrop_action    last_drag_action_handled_ = DNDNONE;

    ~dragndrop_async_writer() {
      sender_.close();
      if (thread_.joinable())
        thread_.join();
    }
  };
  std::unordered_map<wl_data_source *, std::shared_ptr<dragndrop_async_writer>> dragndrop_async_writers_;

  struct clipboard_async_writer {
    std::thread      thread_;
    clipboard_sender sender_;

    ~clipboard_async_writer() {
      sender_.close();
      if (thread_.joinable())
        thread_.join();
    }
  };
  struct clipboard_sender_context {
    send_clipboard_data               callback_;
    wl_data_source                   *selection_ = nullptr;
    std::list<clipboard_async_writer> writers_;

    ~clipboard_sender_context() { clear(); }

    void clear() {
      for (auto &writer : writers_) {
        writer.sender_.close();
        if (writer.thread_.joinable())
          writer.thread_.join();
      }
      if (selection_) {
        wl_data_source_destroy(selection_);
      }
      selection_ = nullptr;
    }
    void reset(wl_data_source *_selection, send_clipboard_data &&_callback) {
      clear();
      selection_ = _selection;
      callback_  = std::move(_callback);
    }
  };
  clipboard_sender_context         clipboard_sender_context_;
  std::list<display::async_reader> async_readers_;
};

}  // namespace hut
