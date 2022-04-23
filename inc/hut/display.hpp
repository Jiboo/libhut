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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <span>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>

#include "hut/utils/flagged.hpp"
#include "hut/utils/fwd.hpp"
#include "hut/utils/length.hpp"
#include "hut/utils/math.hpp"
#include "hut/utils/sstream.hpp"

#include "xdg-shell-client-protocol.h"

namespace hut {

/** Unique ID of a physical keyboard key */
using keycode = u32;

/** ID of function/character, which depends on keymap */
enum keysym {
  KSYM_INVALID,
#define HUT_MAP_KEYSYM(FORMAT_LINUX, FORMAT_X11, FORMAT_IMGUI) KSYM_##FORMAT_LINUX,
#include "hut/keysyms.inc"
#undef HUT_MAP_KEYSYM
  KSYM_LAST_VALUE = KSYM_RIGHTMETA,
};
const char          *keysym_name(keysym _keysym);
inline std::ostream &operator<<(std::ostream &_os, keysym _k) {
  return _os << keysym_name(_k);
}

enum cursor_type {
  CNONE,
  CDEFAULT,

  CCONTEXT_MENU,
  CHELP,
  CPOINTER,
  CPROGRESS,
  CWAIT,

  CCELL,
  CCROSSHAIR,
  CTEXT,

  CALIAS,
  CCOPY,
  CMOVE,
  CNO_DROP,
  CNOT_ALLOWED,
  CGRAB,
  CGRABBING,

  CSCROLL_ALL,
  CRESIZE_COL,
  CRESIZE_ROW,
  CRESIZE_N,
  CRESIZE_E,
  CRESIZE_S,
  CRESIZE_W,
  CRESIZE_NE,
  CRESIZE_NW,
  CRESIZE_SE,
  CRESIZE_SW,
  CRESIZE_EW,
  CRESIZE_NS,
  CRESIZE_NESW,
  CRESIZE_NWSE,

  CZOOM_IN,
  CZOOM_OUT,
  CURSOR_TYPE_LAST_VALUE = CZOOM_OUT,
};
const char          *cursor_css_name(cursor_type _c);
inline std::ostream &operator<<(std::ostream &_os, cursor_type _c) {
  return _os << cursor_css_name(_c);
}

enum clipboard_format {
  FIMAGE_PNG,
  FIMAGE_JPEG,
  FIMAGE_BMP,
  FTEXT_HTML,
  FTEXT_URI_LIST,
  FTEXT_PLAIN,
  CLIPBOARD_FORMAT_LAST_VALUE = FTEXT_PLAIN,
};
using clipboard_formats = flagged<clipboard_format, CLIPBOARD_FORMAT_LAST_VALUE>;
const char          *format_mime_type(clipboard_format _f);
inline std::ostream &operator<<(std::ostream &_os, clipboard_format _f) {
  return _os << format_mime_type(_f);
}
std::optional<clipboard_format> mime_type_format(const char *_mime_type);

enum dragndrop_action {
  DNDNONE,
  DNDCOPY,
  DNDMOVE,
  DRAGNDROP_ACTION_LAST_VALUE = DNDMOVE,
};
using dragndrop_actions = flagged<dragndrop_action, DRAGNDROP_ACTION_LAST_VALUE>;
const char          *action_name(dragndrop_action _a);
inline std::ostream &operator<<(std::ostream &_os, dragndrop_action _a) {
  return _os << action_name(_a);
}

enum modifier {
  KMOD_ALT,
  KMOD_CTRL,
  KMOD_SHIFT,
  MODIFIER_LAST_VALUE = KMOD_SHIFT,
};
using modifiers = flagged<modifier, MODIFIER_LAST_VALUE>;
const char          *modifier_name(modifier _m);
inline std::ostream &operator<<(std::ostream &_os, modifier _a) {
  return _os << modifier_name(_a);
}

class clipboard_sender {
  friend class window;
  friend class display;

  int fd_ = 0;

  void open(int _fd);

 public:
  void    close();
  ssize_t write(std::span<const u8> _data) const;
};

class clipboard_receiver {
  friend class window;
  friend class display;

  int fd_ = 0;

  void open(int _fd);

 public:
  void    close();
  ssize_t read(std::span<u8> _data) const;
};

struct drop_target_interface {
  struct move_result {
    dragndrop_actions possible_actions_;
    dragndrop_action  preferred_action_ = DNDNONE;
    clipboard_format  preferred_format_ = FTEXT_PLAIN;

    constexpr bool operator==(const move_result &) const = default;
  };

  virtual void        on_enter(dragndrop_actions, clipboard_formats)  = 0;
  virtual move_result on_move(vec2)                                   = 0;
  virtual void        on_drop(dragndrop_action, clipboard_receiver &) = 0;
};

class display {
  friend class buffer;
  friend class offscreen;
  friend class window;
  friend class image;
  friend struct details::buffer_page_data;

 public:
  using clock      = std::chrono::steady_clock;
  using time_point = clock::time_point;
  using duration   = clock::duration;
  using callback   = std::function<void(time_point)>;

  display() = delete;

  display(const display &)            = delete;
  display &operator=(const display &) = delete;

  display(display &&) noexcept            = delete;
  display &operator=(display &&) noexcept = delete;

  explicit display(const char *_app_name, u32 _app_version = VK_MAKE_VERSION(1, 0, 0),
                   const char *_display_name = nullptr);
  ~display();

  void flush();
  void flush_staged();
  void roundtrip();
  int  dispatch();

  VkInstance                        instance() { return instance_; }
  VkPhysicalDevice                  pdevice() { return pdevice_; }
  VkDevice                          device() { return device_; }
  const VkPhysicalDeviceFeatures   &features() const { return device_features_; }
  const VkPhysicalDeviceProperties &properties() const { return device_props_; }
  const VkPhysicalDeviceLimits     &limits() const { return device_props_.limits; }

  void post(const callback &_callback);

  char32_t keycode_idle_char(keycode _in) const;
  char    *keycode_name(std::span<char> _out, keycode _in) const;

  template<typename TSignature>
  TSignature get_proc(const std::string &_name) {
    return (TSignature)get_proc_impl(_name);
  }

  [[nodiscard]] u16vec2_px max_tex_size() const {
    auto dim = limits().maxImageDimension2D;
    return {dim, dim};
  }
  [[nodiscard]] uint ubo_align() const { return limits().minUniformBufferOffsetAlignment; }

  std::pair<u32, VkMemoryPropertyFlags> find_memory_type(u32 _type_filter, VkMemoryPropertyFlags _properties);

 protected:
  VkInstance               instance_ = VK_NULL_HANDLE;
  VkDebugReportCallbackEXT debug_cb_ = VK_NULL_HANDLE;

  VkPhysicalDevice                 pdevice_;
  u32                              iqueueg_, iqueuec_, iqueuet_, iqueuep_;
  VkDevice                         device_ = VK_NULL_HANDLE;
  VkSurfaceFormatKHR               surface_format_;
  VkPhysicalDeviceFeatures         device_features_;
  VkPhysicalDeviceProperties       device_props_;
  VkQueue                          queueg_, queuec_, queuet_, queuep_;
  VkCommandPool                    commandg_pool_ = VK_NULL_HANDLE;
  VkPhysicalDeviceMemoryProperties mem_props_;

  void init_vulkan_instance(const char *_app_name, u32 _app_version, std::vector<const char *> &_extensions);
  void init_vulkan_device(VkSurfaceKHR _dummy);
  void destroy_vulkan();

  void *get_proc_impl(const std::string &_name);

  shared_buffer   staging_;
  VkCommandBuffer staging_cb_;
  uint            staging_jobs_ = 0;
  std::mutex      staging_mutex_;
  using flush_callback = std::function<void()>;
  std::vector<flush_callback>      preflush_jobs_;
  std::vector<buffer_suballoc<u8>> postflush_garbage_;

  inline void preflush(const flush_callback &_callback) { preflush_jobs_.emplace_back(_callback); }
  void        postflush_collect(buffer_suballoc<u8> &&_callback);

  struct buffer_copy : public VkBufferCopy {
    VkBuffer source_;
    VkBuffer destination_;
  };

  struct buffer_zero {
    VkBuffer     destination_;
    VkDeviceSize size_;
    VkDeviceSize offset_;
  };

  struct buffer_image_copy : public VkBufferImageCopy {
    VkBuffer source_;
    VkImage  destination_;
    uint     bytes_size_;
  };

  struct image_copy : public VkImageCopy {
    VkImage source_;
    VkImage destination_;
  };

  struct image_clear {
    VkImage           destination_;
    VkClearColorValue color_;
  };

  struct image_transition {
    VkImage       destination_;
    VkImageLayout old_layout_, new_layout_;
  };

  void stage_copy(const buffer_copy &_info);
  void stage_zero(const buffer_zero &_info);

  void stage_copy(const image_copy &_info);
  void stage_copy(const buffer_image_copy &_info);
  void stage_transition(const image_transition &_info, VkImageSubresourceRange _range);
  void stage_clear(const image_clear &_info, VkImageSubresourceRange _range);

  static void transition_image(VkCommandBuffer _cb, VkImage _image, VkImageSubresourceRange _range,
                               VkImageLayout _old_layout, VkImageLayout _new_layout);

  std::list<callback> posted_jobs_;
  std::mutex          posted_mutex_;
  void                process_posts(time_point _now);
  void                post_empty_event();

  static void registry_handler(void *_data, wl_registry *_registry, u32 _id, const char *_interface, u32 _version);
  static void seat_handler(void *_data, wl_seat *_seat, u32 _caps);
  static void pointer_handle_enter(void *_data, wl_pointer *_pointer, u32 _serial, wl_surface *_surface, wl_fixed_t _sx,
                                   wl_fixed_t _sy);
  static void pointer_handle_leave(void *_data, wl_pointer *_pointer, u32 _serial, wl_surface *_surface);
  static void pointer_handle_motion(void *_data, wl_pointer *_pointer, u32 _time, wl_fixed_t _sx, wl_fixed_t _sy);
  static void pointer_handle_button(void *_data, wl_pointer *_pointer, u32 _serial, u32 _time, u32 _button, u32 _state);
  static void pointer_handle_axis(void *_data, wl_pointer *_pointer, u32 _time, u32 _axis, wl_fixed_t _value);
  static void pointer_handle_frame(void *_data, wl_pointer *_pointer);
  static void pointer_handle_axis_source(void *_data, wl_pointer *_pointer, u32 _source);
  static void pointer_handle_axis_stop(void *_data, wl_pointer *_pointer, u32 _time, u32 _axis);
  static void pointer_handle_axis_discrete(void *_data, wl_pointer *_pointer, u32 _axis, i32 _discrete);
  static void keyboard_handle_keymap(void *_data, wl_keyboard *_keyboard, u32 _format, int _fd, u32 _size);
  static void keyboard_handle_enter(void *_data, wl_keyboard *_keyboard, u32 _serial, wl_surface *_surface,
                                    wl_array *_unused);
  static void keyboard_handle_leave(void *_data, wl_keyboard *_keyboard, u32 _serial, wl_surface *_surface);
  static void keyboard_handle_key(void *_data, wl_keyboard *_keyboard, u32 _serial, u32 _time, u32 _key, u32 _state);
  static void keyboard_handle_modifiers(void *_data, wl_keyboard *_keyboard, u32 _serial, u32 _mods_depressed,
                                        u32 _mods_latched, u32 _mods_locked, u32 _group);
  static void keyboard_handle_repeat_info(void *_data, wl_keyboard *_keyboard, i32 _rate, i32 _delay);
  static void data_offer_handle_offer(void *_data, wl_data_offer *_offer, const char *_mime);
  static void data_offer_handle_source_actions(void *_data, wl_data_offer *_offer, u32 _actions);
  static void data_offer_handle_action(void *_data, wl_data_offer *_offer, u32 _action);
  static void data_device_handle_data_offer(void *_data, wl_data_device *_unused, wl_data_offer *_offer);
  static void data_device_handle_enter(void *_data, wl_data_device *_device, u32 _serial, wl_surface *_surface,
                                       wl_fixed_t _x, wl_fixed_t _y, wl_data_offer *_offer);
  static void data_device_handle_leave(void *_data, wl_data_device *_device);
  static void data_device_handle_motion(void *_data, wl_data_device *_device, u32 _time, wl_fixed_t _x, wl_fixed_t _y);
  static void data_device_handle_drop(void *_data, wl_data_device *_device);
  static void data_device_handle_selection(void *_data, wl_data_device *_device, wl_data_offer *_offer);
  static void output_geometry(void *_data, wl_output *_output, int32_t _x, int32_t _y, int32_t _pwidth,
                              int32_t _pheight, int32_t _subpixel, const char *_maker, const char *_model,
                              int32_t _transform);
  static void output_mode(void *_data, wl_output *_output, uint32_t _flags, int32_t _width, int32_t _height,
                          int32_t _refresh);
  static void output_scale(void *_data, wl_output *_output, int32_t _scale);

  struct animate_cursor_context {
    display                &display_;
    std::thread             thread_;
    std::mutex              mutex_;
    std::condition_variable cv_;
    bool                    stop_request_ = false;

    wl_cursor *cursor_ = nullptr;
    size_t     frame_  = 0;

    explicit animate_cursor_context(display &_parent)
        : display_(_parent) {}
  };
  void             cursor(cursor_type _c, u32 _scale);
  void             cursor_frame(wl_cursor *_cursor, size_t _frame);
  void             animate_cursor(wl_cursor *_cursor);
  wl_cursor_theme *cursor_theme_load(u32 _scale);
  static void      animate_cursor_thread(animate_cursor_context *_ctx);

  struct keyboard_repeat_context {
    display                &display_;
    std::thread             thread_;
    std::mutex              mutex_;
    std::condition_variable cv_;
    bool                    stop_request_ = false;

    duration   delay_ = duration::zero();
    duration   sleep_ = duration::zero();
    char32_t   key_   = 0;
    time_point start_;

    explicit keyboard_repeat_context(display &_parent)
        : display_(_parent) {}
  };
  void        keyboard_repeat(char32_t _c);
  static void keyboard_repeat_thread(keyboard_repeat_context *_ctx);

  static keysym map_xkb_keysym(xkb_keysym_t _in);

  std::unordered_map<wl_surface *, window *> windows_;
  bool                                       loop_ = true;

  wl_display    *display_;
  wl_registry   *registry_    = nullptr;
  wl_compositor *compositor_  = nullptr;
  xdg_wm_base   *xdg_wm_base_ = nullptr;
  wl_shm        *shm_         = nullptr;
  wl_seat       *seat_        = nullptr;
  wl_pointer    *pointer_     = nullptr;
  wl_keyboard   *keyboard_    = nullptr;
  wl_output     *output_      = nullptr;
  xkb_context   *xkb_context_ = nullptr;
  xkb_state     *xkb_state_ = nullptr, *xkb_state_empty_ = nullptr;
  xkb_keymap    *keymap_ = nullptr;

  u32 mod_index_alt_           = 0;
  u32 mod_index_ctrl_          = 0;
  u32 mod_index_shift_         = 0;
  u32 last_serial_             = 0;
  u32 last_mouse_enter_serial_ = 0;
  u32 last_mouse_click_serial_ = 0;

  std::unordered_map<wl_output *, u32> outputs_scale_;

  std::pair<wl_surface *, window *> pointer_current_{nullptr, nullptr};
  std::pair<wl_surface *, window *> keyboard_current_{nullptr, nullptr};
  keyboard_repeat_context           keyboard_repeat_ctx_;

  std::mutex                                  cursor_themes_mutex_;
  std::unordered_map<uint, wl_cursor_theme *> cursor_themes_;
  wl_surface                                 *cursor_surface_ = nullptr;
  animate_cursor_context                      animate_cursor_ctx_;

  wl_data_device_manager *data_device_manager_ = nullptr;
  wl_data_device         *data_device_         = nullptr;
  struct offer_params {
    clipboard_formats formats_;
    dragndrop_actions actions_;
    dragndrop_action  drop_action_;
    clipboard_format  drop_format_;
  };
  std::unordered_map<wl_data_offer *, offer_params> offer_params_;
  wl_data_offer                                    *last_offer_from_clipboard_ = nullptr;
  wl_data_offer                                    *last_offer_from_dropenter_ = nullptr;
  window                                           *current_drop_target_       = nullptr;
  u32                                               drag_enter_serial_         = 0;

  struct async_reader {
    std::thread        thread_;
    clipboard_receiver receiver_;

    ~async_reader() {
      receiver_.close();
      if (thread_.joinable())
        thread_.join();
    }
  };
  std::list<async_reader> current_drop_readers_;
};

}  // namespace hut
