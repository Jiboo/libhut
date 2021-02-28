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
#include <map>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-client-protocol-v1.h"
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#include <windows.h>
#else
#error "can't find a suitable VK_USE_PLATFORM macro"
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#include "hut/buffer_pool.hpp"
#include "hut/utils.hpp"

namespace hut {

/** Unique ID of a physical keyboard key */
using keycode = uint32_t;

/** ID of function/character, which depends on keymap */
enum keysym {
  KSYM_INVALID,
#define HUT_MAP_KEYSYM(KEYCODE) KSYM_##KEYCODE,
#include "hut/keysyms.inc"
#undef HUT_MAP_KEYSYM
  KSYM_LAST_VALUE = KSYM_RIGHTMETA,
};
const char *keysym_name(keysym);
inline std::ostream &operator<<(std::ostream &_os, keysym _k) { return _os << keysym_name(_k); }

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
const char *cursor_css_name(cursor_type);
inline std::ostream &operator<<(std::ostream &_os, cursor_type _c) { return _os << cursor_css_name(_c); }

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
const char *format_mime_type(clipboard_format _f);
inline std::ostream &operator<<(std::ostream &_os, clipboard_format _f) { return _os << format_mime_type(_f); }
std::optional<clipboard_format> mime_type_format(const char * _mime_type);

enum dragndrop_action {
  DNDNONE,
  DNDCOPY,
  DNDMOVE,
  DRAGNDROP_ACTION_LAST_VALUE = DNDMOVE,
};
using dragndrop_actions = flagged<dragndrop_action, DRAGNDROP_ACTION_LAST_VALUE>;
const char *action_name(dragndrop_action _a);
inline std::ostream &operator<<(std::ostream &_os, dragndrop_action _a) { return _os << action_name(_a); }

enum modifier {
  KMOD_ALT,
  KMOD_CTRL,
  KMOD_SHIFT,
  MODIFIER_LAST_VALUE = KMOD_SHIFT,
};
using modifiers = flagged<modifier, MODIFIER_LAST_VALUE>;
const char *modifier_name(modifier _m);
inline std::ostream &operator<<(std::ostream &_os, modifier _a) { return _os << modifier_name(_a); }

struct clipboard_sender {
  ssize_t write(span<uint8_t>);

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
  int fd_;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  std::vector<uint8_t> buffer_;
#endif
};
struct clipboard_receiver {
  ssize_t read(span<uint8_t>);

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
  int fd_;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  span<uint8_t> buffer_;
  size_t offset_ = 0;
#endif
};

struct drop_target_interface {
  struct move_result {
    dragndrop_actions possible_actions_;
    dragndrop_action preferred_action_;
    clipboard_format preferred_format_;
  };

  virtual void on_enter(dragndrop_actions, clipboard_formats) = 0;
  virtual move_result on_move(vec2) = 0;
  virtual void on_drop(dragndrop_action, clipboard_receiver&) = 0;
};

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
  void registry_handler(void*, wl_registry*, uint32_t, const char*, uint32_t);
  void seat_handler(void*, wl_seat*, uint32_t);
  void handle_xdg_configure(void*, xdg_surface*, uint32_t);
  void handle_toplevel_decoration_configure(void*, zxdg_toplevel_decoration_v1*, uint32_t);
  void handle_toplevel_configure(void*, xdg_toplevel*, int32_t, int32_t, wl_array*);
  void handle_toplevel_close(void*, xdg_toplevel*);
  void pointer_handle_enter(void*, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t);
  void pointer_handle_leave(void*, wl_pointer*, uint32_t, wl_surface*);
  void pointer_handle_motion(void*, wl_pointer*, uint32_t, wl_fixed_t, wl_fixed_t);
  void pointer_handle_button(void*, wl_pointer*, uint32_t, uint32_t, uint32_t, uint32_t);
  void pointer_handle_axis(void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t);
  void pointer_handle_frame(void*, wl_pointer*);
  void pointer_handle_axis_source(void*, wl_pointer*, uint32_t);
  void pointer_handle_axis_stop(void*, wl_pointer*, uint32_t, uint32_t);
  void pointer_handle_axis_discrete(void*, wl_pointer*, uint32_t, int32_t);
  void keyboard_handle_keymap(void*, wl_keyboard*, uint32_t, int, uint32_t);
  void keyboard_handle_enter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*);
  void keyboard_handle_leave(void*, wl_keyboard*, uint32_t, wl_surface*);
  void keyboard_handle_key(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t);
  void keyboard_handle_modifiers(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
  void keyboard_handle_repeat_info(void*, wl_keyboard*, int32_t, int32_t);
  void clipboard_data_source_handle_target(void*, wl_data_source*, const char*);
  void clipboard_data_source_handle_send(void*, wl_data_source*, const char*, int32_t);
  void clipboard_data_source_handle_cancelled(void*, wl_data_source*);
  void clipboard_data_source_handle_dnd_drop_performed(void*, wl_data_source*);
  void clipboard_data_source_handle_dnd_finished(void*, wl_data_source*);
  void clipboard_data_source_handle_action(void*, wl_data_source*, uint32_t);
  void drag_data_source_handle_target(void*, wl_data_source*, const char*);
  void drag_data_source_handle_send(void*, wl_data_source*, const char*, int32_t);
  void drag_data_source_handle_cancelled(void*, wl_data_source*);
  void drag_data_source_handle_dnd_drop_performed(void*, wl_data_source*);
  void drag_data_source_handle_dnd_finished(void*, wl_data_source*);
  void drag_data_source_handle_action(void*, wl_data_source*, uint32_t);
  void data_offer_handle_offer(void*, wl_data_offer*, const char*);
  void data_offer_handle_source_actions(void*, wl_data_offer*, uint32_t);
  void data_offer_handle_action(void*, wl_data_offer*, uint32_t);
  void data_device_handle_data_offer(void*, wl_data_device*, wl_data_offer*);
  void data_device_handle_enter(void*, wl_data_device*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t, wl_data_offer*);
  void data_device_handle_leave(void*, wl_data_device*);
  void data_device_handle_motion(void*, wl_data_device*, uint32_t, wl_fixed_t, wl_fixed_t);
  void data_device_handle_drop(void*, wl_data_device*);
  void data_device_handle_selection(void*, wl_data_device*, wl_data_offer*);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _umsg, WPARAM _wparam, LPARAM _lparam);
#endif

class display {
  friend class window;
  friend class buffer_pool;
  friend class image;
  friend class sampler;
  friend class font;
  template<typename TUBO, typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments> friend class pipeline;

 public:
  using clock = std::chrono::steady_clock;
  using time_point = clock::time_point;
  using duration = clock::duration;
  using callback = std::function<void(time_point)>;
  using scheduled_item = std::tuple<callback, duration>;

  explicit display(const char *_app_name, uint32_t _app_version = VK_MAKE_VERSION(1, 0, 0),
                   const char *_display_name = nullptr);
  ~display();

  void flush();
  void flush_staged();
  int dispatch();

  void post(const callback &_callback);

  char32_t keycode_idle_char(keycode _in) const;
  char *keycode_name(span<char> _out, keycode _in) const;

  template <typename T>
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

  const VkPhysicalDeviceLimits &limits() const {
    return device_props_.limits;
  }

  constexpr static auto GENERAL_FLAGS = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                                                          | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                                                          | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  shared_buffer alloc_buffer(uint _byte_size,
      VkMemoryPropertyFlags _type = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VkBufferUsageFlagBits _flags = GENERAL_FLAGS);

  template<typename T>
  shared_ref<T> alloc_ubo(shared_buffer &_buf, const T &_default = {}) {
    auto ref = _buf->allocate<T>(1, device_props_.limits.minUniformBufferOffsetAlignment);
    ref->set(_default);
    return ref;
  }

 protected:
  FT_Library ft_library_ = nullptr;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugReportCallbackEXT debug_cb_ = VK_NULL_HANDLE;

  VkPhysicalDevice pdevice_;
  uint32_t iqueueg_, iqueuec_, iqueuet_, iqueuep_;
  VkDevice device_ = VK_NULL_HANDLE;
  VkPhysicalDeviceFeatures device_features_;
  VkPhysicalDeviceProperties device_props_;
  VkQueue queueg_, queuec_, queuet_, queuep_;
  VkCommandPool commandg_pool_ = VK_NULL_HANDLE;
  VkPhysicalDeviceMemoryProperties mem_props_;

  void init_vulkan_instance(const char *_app_name, uint32_t _app_version, std::vector<const char *> &_extensions);
  void init_vulkan_device(VkSurfaceKHR _dummy);
  void destroy_vulkan();

  std::pair<uint32_t, VkMemoryPropertyFlags> find_memory_type(uint32_t _type_filter, VkMemoryPropertyFlags _properties);

  std::shared_ptr<buffer_pool> staging_;
  VkCommandBuffer staging_cb_;
  uint staging_jobs_ = 0;
  std::mutex staging_mutex_;
  using flush_callback = std::function<void()>;
  std::vector<flush_callback> preflush_jobs_;
  std::vector<flush_callback> postflush_jobs_;

  inline void preflush(const flush_callback &_callback) {
    preflush_jobs_.emplace_back(_callback);
  }
  inline void postflush(const flush_callback &_callback) {
    postflush_jobs_.emplace_back(_callback);
  }

  struct buffer_copy : public VkBufferCopy {
    VkBuffer source;
    VkBuffer destination;
  };

  struct buffer_zero {
    VkBuffer destination;
    VkDeviceSize size;
    VkDeviceSize offset;
  };

  struct buffer2image_copy : public VkBufferImageCopy {
    VkBuffer source;
    VkImage destination;
    uint pixelSize;
  };

  struct image_copy : public VkImageCopy {
    VkImage source;
    VkImage destination;
  };

  struct image_clear {
    VkImage destination;
    VkClearColorValue color;
  };

  struct image_transition {
    VkImage destination;
    VkImageLayout oldLayout, newLayout;
  };

  void stage_transition(const image_transition &_info);
  void stage_copy(const buffer_copy &_info);
  void stage_zero(const buffer_zero &_info);
  void stage_copy(const image_copy &_info);
  void stage_copy(const buffer2image_copy &_info);
  void stage_clear(const image_clear &_info);

  std::list<callback> posted_jobs_;
  std::mutex posted_mutex_;
  void process_posts(time_point _now);
  void post_empty_event();

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

  struct animate_cursor_context {
    display &display_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_request_ = false;

    wl_cursor *cursor_ = nullptr;
    size_t frame_ = 0;
  };
  void cursor(cursor_type _c);
  void cursor_frame(wl_cursor *_cursor, size_t _frame);
  void animate_cursor(wl_cursor *_cursor);
  static void animate_cursor_thread(animate_cursor_context *_ctx);

  struct keyboard_repeat_context {
    display &display_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_request_ = false;

    duration delay_;
    duration sleep_;
    char32_t key_ = 0;
    time_point start_;
  };
  void keyboard_repeat(char32_t _c);
  static void keyboard_repeat_thread(keyboard_repeat_context *_ctx);

  static keysym map_xkb_keysym(xkb_keysym_t);

  std::unordered_map<wl_surface*, window*> windows_;
  bool loop_ = true;

  wl_display *display_;
  wl_registry *registry_ = nullptr;
  wl_compositor *compositor_ = nullptr;
  xdg_wm_base *xdg_wm_base_ = nullptr;
  wl_shm *shm_ = nullptr;
  zxdg_decoration_manager_v1 *decoration_manager_ = nullptr;

  wl_seat *seat_ = nullptr;
  wl_pointer *pointer_ = nullptr;
  wl_keyboard *keyboard_ = nullptr;
  xkb_context *xkb_context_ = nullptr;
  xkb_state *xkb_state_ = nullptr, *xkb_state_empty_ = nullptr;
  xkb_keymap *keymap_ = nullptr;
  uint32_t mod_index_alt_ = 0;
  uint32_t mod_index_ctrl_ = 0;
  uint32_t mod_index_shift_ = 0;
  uint32_t last_serial_ = 0;
  std::pair<wl_surface*, window*> pointer_current_ {nullptr, nullptr};
  std::pair<wl_surface*, window*> keyboard_current_ {nullptr, nullptr};
  keyboard_repeat_context keyboard_repeat_ctx_;

  wl_cursor_theme *cursor_theme_ = nullptr;
  wl_surface *cursor_surface_ = nullptr;
  animate_cursor_context animate_cursor_ctx_;

  wl_data_device_manager *data_device_manager_ = nullptr;
  wl_data_device *data_device_ = nullptr;
  struct offer_params {
    clipboard_formats formats_;
    dragndrop_actions actions_;
    dragndrop_action drop_action_;
    clipboard_format drop_format_;
  };
  std::unordered_map<wl_data_offer*, offer_params> offer_params_;
  wl_data_offer *last_offer_from_clipboard_ = nullptr;
  wl_data_offer *last_offer_from_dropenter_ = nullptr;
  std::shared_ptr<drop_target_interface> current_drop_target_interface_;
  uint32_t drag_enter_serial_ = 0;

#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  static std::string utf8_wstr(const WCHAR *_input);
  friend LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _umsg, WPARAM _wparam, LPARAM _lparam);

  UINT format_win32(clipboard_format _format);
  std::optional<clipboard_format> win32_format(UINT _format);

  #define HUT_WIN32_CLASSNAME_CSD "hut_csd"
  #define HUT_WIN32_CLASSNAME_SSD "hut_ssd"
  HINSTANCE hinstance_;
  std::unordered_map<HWND, window *> windows_;
  UINT html_clipboard_format_;
#endif
};

}  // namespace hut
