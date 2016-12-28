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
#endif

#include "hut/buffer.hpp"
#include "hut/utils.hpp"

namespace hut {

class window;
class display;
class buffer;

class display {
  friend class buffer;
  friend class window;
  friend class noinput;
  friend class colored_triangle_list;

 public:
  using clock = std::chrono::steady_clock;
  using time_point = clock::time_point;
  using duration = clock::duration;
  using callback = std::function<void(time_point)>;
  using scheduled_item = std::tuple<callback, duration>;

  display(const char *_app_name,
          uint32_t _app_version = VK_MAKE_VERSION(1, 0, 0),
          const char *_display_name = nullptr);
  ~display();

  void flush();
  int dispatch();

  void post(callback _callback);
  void post_overridable(callback _callback, size_t _id);
  void post_delayed(callback _callback, std::chrono::milliseconds _delay);

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

 protected:
  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugReportCallbackEXT debug_cb_ = VK_NULL_HANDLE;

  VkPhysicalDevice pdevice_;
  uint32_t iqueueg_, iqueuec_, iqueuet_, iqueuep_;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue queueg_, queuec_, queuet_, queuep_;
  VkCommandPool commandg_pool_ = VK_NULL_HANDLE;
  VkPhysicalDeviceMemoryProperties mem_props_;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

  std::shared_ptr<buffer> staging_;
  VkCommandBuffer staging_cb_;
  bool dirty_staging_ = false;

  std::list<callback> posted_jobs_;
  std::map<size_t, callback> overridable_jobs_;
  std::multimap<time_point, callback> delayed_jobs_;
  std::mutex posted_mutex_, overridable_mutex_, delayed_mutex_;
  std::mutex cv_mutex_;
  std::condition_variable cv_;
  std::thread::id dispatcher_;

  void init_vulkan_instance(const char *_app_name, uint32_t _app_version,
                            std::vector<const char *> &extensions);
  void init_vulkan_device(VkSurfaceKHR dummy);
  std::pair<uint32_t, VkMemoryPropertyFlags> find_memory_type(
      uint32_t typeFilter, VkMemoryPropertyFlags properties);
  void stage_update(VkBuffer _src, VkBufferCopy *_info);
  void flush_staged_copies();
  void destroy_vulkan();

  time_point next_job_time_point();

  void tick_posted(time_point _now);
  void tick_overridable(time_point _now);
  void tick_delayed(time_point _now);
  void jobs_loop();
  void check_thread();

#if defined(VK_USE_PLATFORM_XCB_KHR)
  xcb_connection_t *connection_;
  xcb_screen_t *screen_;
  xcb_key_symbols_t *keysyms_;
  std::unordered_map<xcb_window_t, window *> windows_;

  xcb_atom_t atom_wm_, atom_close_;
#endif
};

}  // namespace hut
