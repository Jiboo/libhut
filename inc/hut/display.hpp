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
const char          *keysym_name(keysym);
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
const char          *cursor_css_name(cursor_type);
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
  ssize_t write(std::span<const u8>);
};

class clipboard_receiver {
  friend class window;
  friend class display;

  int fd_ = 0;

  void open(int _fd);

 public:
  void    close();
  ssize_t read(std::span<u8>);
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
  friend class details::buffer_page_data;

 public:
  using clock          = std::chrono::steady_clock;
  using time_point     = clock::time_point;
  using duration       = clock::duration;
  using callback       = std::function<void(time_point)>;
  using scheduled_item = std::tuple<callback, duration>;

  display() = delete;

  display(const display &) = delete;
  display &operator=(const display &) = delete;

  display(display &&) noexcept = delete;
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

  template<typename T>
  T get_proc(const std::string &_name) {
    static std::unordered_map<std::string, void *> cache;

    auto it = cache.find(_name);
    if (it == cache.end()) {
      auto func = (T)vkGetInstanceProcAddr(instance_, _name.data());
      if (func == nullptr)
        throw std::runtime_error(sstream("can't find address of ") << _name);
      cache.emplace(_name, (void *)func);
      return func;
    } else {
      return (T)it->second;
    }
  }

  [[nodiscard]] u16vec2 max_tex_size() const {
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
    VkBuffer source;
    VkBuffer destination;
  };

  struct buffer_zero {
    VkBuffer     destination;
    VkDeviceSize size;
    VkDeviceSize offset;
  };

  struct buffer2image_copy : public VkBufferImageCopy {
    VkBuffer source;
    VkImage  destination;
    uint     bytesSize;
  };

  struct image_copy : public VkImageCopy {
    VkImage source;
    VkImage destination;
  };

  struct image_clear {
    VkImage           destination;
    VkClearColorValue color;
  };

  struct image_transition {
    VkImage       destination;
    VkImageLayout oldLayout, newLayout;
  };

  void stage_copy(const buffer_copy &_info);
  void stage_zero(const buffer_zero &_info);

  void stage_copy(const image_copy &_info);
  void stage_copy(const buffer2image_copy &_info);
  void stage_transition(const image_transition &_info, VkImageSubresourceRange _range);
  void stage_clear(const image_clear &_info, VkImageSubresourceRange _range);

  static void transition_image(VkCommandBuffer _cb, VkImage _image, VkImageSubresourceRange _range,
                               VkImageLayout _oldLayout, VkImageLayout _newLayout);

  std::list<callback> posted_jobs_;
  std::mutex          posted_mutex_;
  void                process_posts(time_point _now);
  void                post_empty_event();

  static void registry_handler(void *, wl_registry *, u32, const char *, u32);
  static void seat_handler(void *, wl_seat *, u32);
  static void pointer_handle_enter(void *, wl_pointer *, u32, wl_surface *, wl_fixed_t, wl_fixed_t);
  static void pointer_handle_leave(void *, wl_pointer *, u32, wl_surface *);
  static void pointer_handle_motion(void *, wl_pointer *, u32, wl_fixed_t, wl_fixed_t);
  static void pointer_handle_button(void *, wl_pointer *, u32, u32, u32, u32);
  static void pointer_handle_axis(void *, wl_pointer *, u32, u32, wl_fixed_t);
  static void pointer_handle_frame(void *, wl_pointer *);
  static void pointer_handle_axis_source(void *, wl_pointer *, u32);
  static void pointer_handle_axis_stop(void *, wl_pointer *, u32, u32);
  static void pointer_handle_axis_discrete(void *, wl_pointer *, u32, i32);
  static void keyboard_handle_keymap(void *, wl_keyboard *, u32, int, u32);
  static void keyboard_handle_enter(void *, wl_keyboard *, u32, wl_surface *, wl_array *);
  static void keyboard_handle_leave(void *, wl_keyboard *, u32, wl_surface *);
  static void keyboard_handle_key(void *, wl_keyboard *, u32, u32, u32, u32);
  static void keyboard_handle_modifiers(void *, wl_keyboard *, u32, u32, u32, u32, u32);
  static void keyboard_handle_repeat_info(void *, wl_keyboard *, i32, i32);
  static void data_offer_handle_offer(void *, wl_data_offer *, const char *);
  static void data_offer_handle_source_actions(void *, wl_data_offer *, u32);
  static void data_offer_handle_action(void *, wl_data_offer *, u32);
  static void data_device_handle_data_offer(void *, wl_data_device *, wl_data_offer *);
  static void data_device_handle_enter(void *, wl_data_device *, u32, wl_surface *, wl_fixed_t, wl_fixed_t,
                                       wl_data_offer *);
  static void data_device_handle_leave(void *, wl_data_device *);
  static void data_device_handle_motion(void *, wl_data_device *, u32, wl_fixed_t, wl_fixed_t);
  static void data_device_handle_drop(void *, wl_data_device *);
  static void data_device_handle_selection(void *, wl_data_device *, wl_data_offer *);
  static void output_geometry(void *, wl_output *, int32_t, int32_t, int32_t, int32_t, int32_t, const char *,
                              const char *, int32_t);
  static void output_mode(void *, wl_output *, uint32_t, int32_t, int32_t, int32_t);
  static void output_scale(void *, wl_output *, int32_t);

  struct animate_cursor_context {
    display                &display_;
    std::thread             thread_;
    std::mutex              mutex_;
    std::condition_variable cv_;
    bool                    stop_request_ = false;

    wl_cursor *cursor_ = nullptr;
    size_t     frame_  = 0;
  };
  void        cursor(cursor_type _c, int _scale);
  void        cursor_frame(wl_cursor *_cursor, size_t _frame);
  void        animate_cursor(wl_cursor *_cursor);
  static void animate_cursor_thread(animate_cursor_context *_ctx);
  static void cursor_themes_load(animate_cursor_context *_ctx);

  struct keyboard_repeat_context {
    display                &display_;
    std::thread             thread_;
    std::mutex              mutex_;
    std::condition_variable cv_;
    bool                    stop_request_ = false;

    duration   delay_;
    duration   sleep_;
    char32_t   key_ = 0;
    time_point start_;
  };
  void        keyboard_repeat(char32_t _c);
  static void keyboard_repeat_thread(keyboard_repeat_context *_ctx);

  static keysym map_xkb_keysym(xkb_keysym_t);

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

  std::unordered_map<wl_output *, int> outputs_scale_;

  std::pair<wl_surface *, window *> pointer_current_{nullptr, nullptr};
  std::pair<wl_surface *, window *> keyboard_current_{nullptr, nullptr};
  keyboard_repeat_context           keyboard_repeat_ctx_;

  std::atomic<wl_cursor_theme *> cursor_theme_       = nullptr;
  std::atomic<wl_cursor_theme *> cursor_theme_hidpi_ = nullptr;
  wl_surface                    *cursor_surface_     = nullptr;
  animate_cursor_context         animate_cursor_ctx_;

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
