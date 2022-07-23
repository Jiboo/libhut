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
#include <unordered_set>

#include <wayland-client.h>

#include "hut/utils/event.hpp"

#include "hut/display.hpp"
#include "hut/render_target.hpp"

namespace hut {

using edge = flagged<side, LAST_SIDE>;
cursor_type edge_cursor(edge _edge);

enum touch_event_type { TDOWN, TUP, TMOVE };
const char          *touch_event_name(touch_event_type _type);
inline std::ostream &operator<<(std::ostream &_os, touch_event_type _c) {
  return _os << touch_event_name(_c);
}

enum mouse_event_type { MDOWN, MUP, MMOVE, MWHEEL_UP, MWHEEL_DOWN };
const char          *mouse_event_name(mouse_event_type _type);
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

class window final : public render_target {
  friend class display;

 public:
  buffered_event<>                          on_pause_;   // sys asked win to stop rendering
  buffered_event<>                          on_resume_;  // sys asked win to resume rendering
  buffered_event<u16vec2_px, u32 /*scale*/> on_resize_;  // sys changed win size or scaling

  event<bool>              on_focus_;   // sys gave keyboard focus to win
  event<>                  on_close_;   // overridable, sys requested win to close
  event<u16bbox_px>        on_expose_;  // sys request win to redraw this rect
  event<VkCommandBuffer>   on_draw_;    // win is dirty, needs to rebuild command buffer
  event<display::duration> on_frame_;   // win will soon be drawn (use this to animate values)

#if defined(HUT_ENABLE_TIME_EVENTS)
  event<u32> on_time_;  // contains timestamps of sys events, to measure hut latency
#endif

  event<u8 /*finger*/, touch_event_type, vec2_px /*pos*/> on_touch_;
  event<u8 /*button*/, mouse_event_type, vec2_px /*pos*/> on_mouse_;

  event<modifiers /*mods*/>             on_kmods_;
  event<keycode, keysym, bool /*down*/> on_key_;
  event<char32_t /*utf32_char*/>        on_char_;

  window() = delete;

  window(const window &)            = delete;
  window &operator=(const window &) = delete;

  window(window &&) noexcept            = delete;
  window &operator=(window &&) noexcept = delete;

  explicit window(display &_display, shared_buffer _storage, const window_params &_init_params = {});
  ~window() final;

  void close();
  void pause();

  void invalidate(const uvec4 &_rect, bool _redraw);
  void invalidate(bool _redraw);
  void maximize(bool _set = true);
  void fullscreen(bool _set = true);

  u16vec2_px size() const { return size_; }
  u32        scale() const { return scale_; }

  void interactive_resize(edge _edge);
  void interactive_move();

  void title(std::u8string_view _title);
  void cursor(cursor_type _c);
  void clear_color(const vec4 &_color) {
    render_target_params_.clear_color_ = {{_color.r, _color.g, _color.b, _color.a}};
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
  shared_buffer storage_;
  window_params params_;

  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  VkPresentModeKHR present_mode_ = VK_PRESENT_MODE_MAX_ENUM_KHR;

  VkExtent2D                   swapchain_extents_;
  VkSwapchainKHR               swapchain_ = VK_NULL_HANDLE;
  std::vector<VkImage>         swapchain_images_;
  std::vector<VkImageView>     swapchain_imageviews_;
  std::vector<VkCommandBuffer> primary_cbs_;
  std::vector<VkCommandBuffer> cbs_;
  std::vector<u8>              dirty_;

  VkSemaphore sem_available_ = VK_NULL_HANDLE;
  VkSemaphore sem_rendered_  = VK_NULL_HANDLE;

  u16vec2_px          size_{0_px};
  u16vec2_px          pos_{0_px};
  u32                 scale_          = 1;
  display::time_point last_frame_     = display::clock::now();
  std::atomic_bool    invalidated_    = true;
  vec2                mouse_lastmove_ = {0, 0};
  bool                minimized_      = false;

  std::shared_ptr<drop_target_interface> drop_target_interface_;

  void init_vulkan_surface();
  void destroy_vulkan();
  void redraw(display::time_point _tp);

  static void surface_enter(void *_data, wl_surface *_surface, wl_output *_output);
  static void surface_leave(void *_data, wl_surface *_surface, wl_output *_output);
  static void handle_xdg_configure(void *_data, xdg_surface *_unused, u32 _serial);
  static void handle_toplevel_configure(void *_data, xdg_toplevel *_unused, i32 _width, i32 _height, wl_array *_states);
  static void handle_toplevel_close(void *_data, xdg_toplevel *_unused);
  static void clipboard_data_source_handle_target(void *_data, wl_data_source *_source, const char *_mime);
  static void clipboard_data_source_handle_send(void *_data, wl_data_source *_source, const char *_mime, i32 _fd);
  static void clipboard_data_source_handle_cancelled(void *_data, wl_data_source *_source);
  static void clipboard_data_source_handle_dnd_drop_performed(void *_data, wl_data_source *_source);
  static void clipboard_data_source_handle_dnd_finished(void *_data, wl_data_source *_source);
  static void clipboard_data_source_handle_action(void *_data, wl_data_source *_source, u32 _action);
  static void drag_data_source_handle_target(void *_data, wl_data_source *_source, const char *_mime);
  static void drag_data_source_handle_send(void *_data, wl_data_source *_source, const char *_mime, i32 _fd);
  static void drag_data_source_handle_cancelled(void *_data, wl_data_source *_source);
  static void drag_data_source_handle_dnd_drop_performed(void *_data, wl_data_source *_source);
  static void drag_data_source_handle_dnd_finished(void *_data, wl_data_source *_source);
  static void drag_data_source_handle_action(void *_data, wl_data_source *_source, u32 _action);

  wl_surface   *wayland_surface_;
  xdg_surface  *window_;
  xdg_toplevel *toplevel_;

  cursor_type current_cursor_type_ = CDEFAULT;

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
      if (selection_ != nullptr) {
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

  std::unordered_set<wl_output *> overlapping_outputs_;
  void                            trigger_scale();
};

}  // namespace hut
