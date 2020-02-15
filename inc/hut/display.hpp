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
#include <vector>

#include <vulkan/vulkan.h>

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb_keysyms.h>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include "xdg-shell-client-protocol.h"
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#include <windows.h>
#else
#error "can't find a suitable VK_USE_PLATFORM macro"
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#include "hut/utils.hpp"

namespace hut {

#if defined(VK_USE_PLATFORM_WIN32_KHR)
LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _umsg, WPARAM _wparam, LPARAM _lparam);
#endif

class display {
  friend class window;
  friend class buffer_pool;
  friend class image;
  friend class sampler;
  friend class font;
  template<typename TDetails, typename... TExtraBindings> friend class drawable;

 public:
  using clock = std::chrono::steady_clock;
  using time_point = clock::time_point;
  using duration = clock::duration;
  using callback = std::function<void(time_point)>;
  using scheduled_item = std::tuple<callback, duration>;

  display(const char *_app_name, uint32_t _app_version = VK_MAKE_VERSION(1, 0, 0), const char *_display_name = nullptr);
  ~display();

  void flush();
  void flush_staged();
  int dispatch();

  void post(callback _callback);

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

  VkPhysicalDeviceLimits limits() {
    return device_props_.limits;
  }

  constexpr static VkBufferUsageFlagBits GENERAL_FLAGS = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                                                          | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                                                          | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  shared_buffer alloc_buffer(uint _byte_size,
      VkMemoryPropertyFlags _type = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VkBufferUsageFlagBits _flags = GENERAL_FLAGS);

 protected:
  FT_Library ft_library_;
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
  std::vector<flush_callback> preflush_jobs_, postflush_jobs_;

  inline void preflush(flush_callback _callback) {
    preflush_jobs_.emplace_back(_callback);
  }

  struct buffer_copy : public VkBufferCopy {
    VkBuffer source;
    VkBuffer destination;
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
  void stage_copy(const image_copy &_info);
  void stage_copy(const buffer2image_copy &_info);
  void stage_clear(const image_clear &_info);

  std::list<callback> posted_jobs_;
  std::mutex posted_mutex_;
  void process_posts(time_point _now);
  void post_empty_event();

#if defined(VK_USE_PLATFORM_XCB_KHR)
  xcb_connection_t *connection_;
  xcb_screen_t *screen_;
  xcb_key_symbols_t *keysyms_;
  std::unordered_map<xcb_window_t, window *> windows_;
  xcb_atom_t atom_wm_, atom_close_, atom_change_state_, atom_state_, atom_rstate_, atom_maximizeh_, atom_maximizev_;
  xcb_atom_t atom_window_hints_;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
  friend void registry_handler(void*, wl_registry*, uint32_t, const char*, uint32_t);
  friend void seat_handler(void*, wl_seat*, uint32_t);
  friend void pointer_handle_enter(void*, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t);
  friend void pointer_handle_leave(void*, wl_pointer*, uint32_t, wl_surface*);
  friend void pointer_handle_motion(void*, wl_pointer*, uint32_t, wl_fixed_t, wl_fixed_t);
  friend void pointer_handle_button(void*, wl_pointer*, uint32_t, uint32_t, uint32_t, uint32_t);
  friend void pointer_handle_axis(void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t);
  friend void keyboard_handle_keymap(void*, wl_keyboard*, uint32_t, int, uint32_t);
  friend void keyboard_handle_enter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*);
  friend void keyboard_handle_leave(void*, wl_keyboard*, uint32_t, wl_surface*);
  friend void keyboard_handle_key(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t);
  friend void keyboard_handle_modifiers(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
  wl_display *display_;
  wl_registry *registry_ = nullptr;
  wl_compositor *compositor_ = nullptr;
  xdg_wm_base *xdg_wm_base_ = nullptr;
  wl_seat *seat_ = nullptr;
  wl_pointer *pointer_ = nullptr;
  wl_keyboard *keyboard_ = nullptr;
  std::unordered_map<wl_surface*, window*> windows_;
  bool loop_ = true;
  std::pair<wl_surface*, window*> pointer_current_ {nullptr, nullptr};
  std::pair<wl_surface*, window*> keyboard_current_ {nullptr, nullptr};
  xkb_context *xkb_context_ = nullptr;
  xkb_state *xkb_state_ = nullptr;
  xkb_keymap *keymap_ = nullptr;
  uint32_t kb_mod_mask_ = 0;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  friend LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _umsg, WPARAM _wparam, LPARAM _lparam);
  #define HUT_WIN32_CLASSNAME L"Hut Window Class"
  HINSTANCE hinstance_;
  std::unordered_map<HWND, window *> windows_;
#endif
};

}  // namespace hut
