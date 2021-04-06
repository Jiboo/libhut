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

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#include <windows.h>
#else
#error "can't find a suitable VK_USE_PLATFORM macro"
#endif

#include "hut/display.hpp"
#include "hut/render_target.hpp"
#include "hut/utils.hpp"

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

enum mouse_event_type { MDOWN, MUP, MMOVE, MWHEEL_UP, MWHEEL_DOWN };

class display;

struct window_params {
  enum flag {
    FDEPTH = render_target_params::flag::FDEPTH,
    FMULTISAMPLING = render_target_params::flag::FMULTISAMPLING,
    FSYSTEM_DECORATIONS,
    FVSYNC,
    FTRANSPARENT,
    FFULLSCREEN,
    FLAG_LAST_VALUE = FFULLSCREEN,
  };
  using flags = flagged<flag, flag::FLAG_LAST_VALUE>;

  flags flags_ {FVSYNC, FSYSTEM_DECORATIONS};
  uvec4 position_ = {0, 0, 800, 600};
  uvec2 min_size_ = {0, 0}, max_size_ = {0, 0};
};

class window : public render_target {
  friend class display;
  template<typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments> friend class pipeline;

 public:
  event<> on_pause, on_resume, on_focus, on_blur, on_close;
  event<uvec4> on_expose;
  event<uvec2> on_resize;
  event<VkCommandBuffer> on_draw;
  event<display::duration> on_frame;

  event<uint8_t /*finger*/, touch_event_type, vec2 /*pos*/> on_touch;
  event<uint8_t /*button*/, mouse_event_type, vec2 /*pos*/> on_mouse;

  event<modifiers /*mods*/> on_kmods;
  event<keycode, keysym, bool /*down*/> on_key;
  event<char32_t /*utf32_char*/> on_char;

  explicit window(display &_display, const window_params &_init_params = {});
  ~window();

  [[nodiscard]] bool has_system_decorations() const { return has_system_decorations_; }

  void close();
  void pause();

  void invalidate(const uvec4 &, bool _redraw);
  void invalidate(bool _redraw);

  void maximize(bool _set = true);
  void fullscreen(bool _set = true);
  uvec2 size() {
    return size_;
  }
  void interactive_resize(edge);
  void interactive_move();

  void title(const std::string &);
  void cursor(cursor_type _c);
  void clear_color(const vec4 &_color) {
    render_target_params_.clear_color_ = {_color.r, _color.g, _color.b, _color.a};
    invalidate(true);
  }

  using send_clipboard_data = std::function<void(clipboard_format /*_selected_format*/, clipboard_sender &)>;
  void clipboard_offer(clipboard_formats _supported_formats, const send_clipboard_data &_callback);

  using receive_clipboard_data = std::function<void(clipboard_format /*selected_format*/, clipboard_receiver &)>;
  bool clipboard_receive(clipboard_formats _supported_formats, const receive_clipboard_data &_callback);

  void dragndrop_target(const std::shared_ptr<drop_target_interface> &_received) { drop_target_interface_ = _received; }

  using send_dragndrop_data = std::function<void(dragndrop_action /*_action*/, clipboard_format /*_selected_format*/, clipboard_sender &)>;
  void dragndrop_start(dragndrop_actions _supported_actions, clipboard_formats _supported_formats, const send_dragndrop_data &_callback);

 protected:
  display &display_;
  window_params params_;

  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  VkPresentModeKHR present_mode_;
  VkSurfaceFormatKHR surface_format_;

  VkExtent2D swapchain_extents_;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_imageviews_;
  std::vector<VkCommandBuffer> primary_cbs_;
  std::vector<VkCommandBuffer> cbs_;
  std::vector<uint8_t> dirty_;

  VkSemaphore sem_available_ = VK_NULL_HANDLE;
  VkSemaphore sem_rendered_ = VK_NULL_HANDLE;

  uvec2 size_, pos_;
  uvec2 previous_size_, previous_pos_; // used to save rect before maximize/fullscreen
  display::time_point last_frame_ = display::clock::now();
  bool minimized_ = false;
  bool has_system_decorations_ = false;

  std::shared_ptr<drop_target_interface> drop_target_interface_;

  void init_vulkan_surface();
  void destroy_vulkan();
  void redraw(display::time_point);

 protected:
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
  friend void registry_handler(void*, wl_registry*, uint32_t, const char*, uint32_t);
  friend void seat_handler(void*, wl_seat*, uint32_t);
  friend void handle_xdg_configure(void*, xdg_surface*, uint32_t);
  friend void handle_toplevel_decoration_configure(void*, zxdg_toplevel_decoration_v1*, uint32_t);
  friend void handle_toplevel_configure(void*, xdg_toplevel*, int32_t, int32_t, wl_array*);
  friend void handle_toplevel_close(void*, xdg_toplevel*);
  friend void pointer_handle_enter(void*, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t);
  friend void pointer_handle_leave(void*, wl_pointer*, uint32_t, wl_surface*);
  friend void pointer_handle_motion(void*, wl_pointer*, uint32_t, wl_fixed_t, wl_fixed_t);
  friend void pointer_handle_button(void*, wl_pointer*, uint32_t, uint32_t, uint32_t, uint32_t);
  friend void pointer_handle_axis(void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t);
  friend void pointer_handle_frame(void*, wl_pointer*);
  friend void pointer_handle_axis_source(void*, wl_pointer*, uint32_t);
  friend void pointer_handle_axis_stop(void*, wl_pointer*, uint32_t, uint32_t);
  friend void pointer_handle_axis_discrete(void*, wl_pointer*, uint32_t, int32_t);
  friend void keyboard_handle_keymap(void*, wl_keyboard*, uint32_t, int, uint32_t);
  friend void keyboard_handle_enter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*);
  friend void keyboard_handle_leave(void*, wl_keyboard*, uint32_t, wl_surface*);
  friend void keyboard_handle_key(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t);
  friend void keyboard_handle_modifiers(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
  friend void keyboard_handle_repeat_info(void*, wl_keyboard*, int32_t, int32_t);
  friend void clipboard_data_source_handle_target(void*, wl_data_source*, const char*);
  friend void clipboard_data_source_handle_send(void*, wl_data_source*, const char*, int32_t);
  friend void clipboard_data_source_handle_cancelled(void*, wl_data_source*);
  friend void clipboard_data_source_handle_dnd_drop_performed(void*, wl_data_source*);
  friend void clipboard_data_source_handle_dnd_finished(void*, wl_data_source*);
  friend void clipboard_data_source_handle_action(void*, wl_data_source*, uint32_t);
  friend void drag_data_source_handle_target(void*, wl_data_source*, const char*);
  friend void drag_data_source_handle_send(void*, wl_data_source*, const char*, int32_t);
  friend void drag_data_source_handle_cancelled(void*, wl_data_source*);
  friend void drag_data_source_handle_dnd_drop_performed(void*, wl_data_source*);
  friend void drag_data_source_handle_dnd_finished(void*, wl_data_source*);
  friend void drag_data_source_handle_action(void*, wl_data_source*, uint32_t);
  friend void data_offer_handle_offer(void*, wl_data_offer*, const char*);
  friend void data_offer_handle_source_actions(void*, wl_data_offer*, uint32_t);
  friend void data_offer_handle_action(void*, wl_data_offer*, uint32_t);
  friend void data_device_handle_data_offer(void*, wl_data_device*, wl_data_offer*);
  friend void data_device_handle_enter(void*, wl_data_device*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t, wl_data_offer*);
  friend void data_device_handle_leave(void*, wl_data_device*);
  friend void data_device_handle_motion(void*, wl_data_device*, uint32_t, wl_fixed_t, wl_fixed_t);
  friend void data_device_handle_drop(void*, wl_data_device*);
  friend void data_device_handle_selection(void*, wl_data_device*, wl_data_offer*);

  wl_surface *wayland_surface_;
  xdg_surface *window_;
  xdg_toplevel *toplevel_;
  wl_data_source *current_selection_ = nullptr, *current_drag_source_ = nullptr;
  zxdg_toplevel_decoration_v1 *decoration_ = nullptr;

  std::atomic_bool invalidated_ = true;
  vec2 mouse_lastmove_ = {0, 0};
  cursor_type current_cursor_type_ = CDEFAULT;
  send_clipboard_data current_clipboard_sender_;
  send_dragndrop_data current_dragndrop_sender_;
  uint32_t last_pointer_button_press_serial_ = 0;
  dragndrop_action last_drag_action_handled_ = DNDNONE;

#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  friend LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _umsg, WPARAM _wparam, LPARAM _lparam);

  void clipboard_write(clipboard_format _format, UINT _win_format);
  span<uint8_t> parse_html_clipboard(std::string_view _input);
  std::vector<uint8_t> format_html_clipboard(span<uint8_t> _input);

  HWND window_;
  vec2 mouse_lastmove_ = {0, 0};
  bool button_pressed_ = false;
  cursor_type current_cursor_type_ = CDEFAULT;
  send_clipboard_data current_clipboard_sender_;
  clipboard_formats current_clipboard_formats_;
#endif
};

}  // namespace hut
