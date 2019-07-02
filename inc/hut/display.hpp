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
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#include "hut/buffer.hpp"
#include "hut/utils.hpp"
#include "image.hpp"

namespace hut {

class window;
class display;
class buffer;

class display {
  friend class window;
  friend class buffer;
  friend class image;
  friend class sampler;
  friend class font;
  friend class rgb;
  friend class rgba;
  friend class tex;
  friend class rgb_tex;
  friend class rgba_tex;
  friend class rgba_mask;

 public:
  using clock = std::chrono::steady_clock;
  using time_point = clock::time_point;
  using duration = clock::duration;
  using callback = std::function<void(time_point)>;
  using scheduled_item = std::tuple<callback, duration>;

  display(const char *_app_name, uint32_t _app_version = VK_MAKE_VERSION(1, 0, 0), const char *_display_name = nullptr);
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
  VkPhysicalDeviceFeatures device_features_;
  VkPhysicalDeviceProperties device_props_;
  VkQueue queueg_, queuec_, queuet_, queuep_;
  VkCommandPool commandg_pool_ = VK_NULL_HANDLE;
  VkPhysicalDeviceMemoryProperties mem_props_;

  std::shared_ptr<buffer> staging_;
  VkCommandBuffer staging_cb_;
  bool dirty_staging_ = false;
  std::atomic<uint> stage_pending_{0};
  std::mutex staging_mutex_;
  using preflush_callback = std::function<void()>;
  std::list<preflush_callback> preflush_jobs_;
  std::mutex preflush_mutex_;

  void flush_staged();
  void preflush(preflush_callback _callback) {
    std::lock_guard lk(preflush_mutex_);
    preflush_jobs_.emplace_back(_callback);
  }

  event<> on_staged;
  std::list<callback> posted_jobs_;
  std::map<size_t, callback> overridable_jobs_;
  std::multimap<time_point, callback> delayed_jobs_;
  std::mutex posted_mutex_, overridable_mutex_, delayed_mutex_;
  std::mutex cv_mutex_;
  std::condition_variable cv_;
  std::thread::id dispatcher_;

  FT_Library ft_library_;

  void init_vulkan_instance(const char *_app_name, uint32_t _app_version, std::vector<const char *> &_extensions);
  void init_vulkan_device(VkSurfaceKHR _dummy);
  std::pair<uint32_t, VkMemoryPropertyFlags> find_memory_type(uint32_t _type_filter, VkMemoryPropertyFlags _properties);
  void stage_copy(VkBuffer _src, VkBuffer _dst, const VkBufferCopy *_info);
  void stage_copy(VkBuffer _dst, const VkBufferCopy *_info);
  void stage_transition(VkImage _image, VkImageLayout _old_layout, VkImageLayout _new_layout);
  void stage_copy(VkImage _src, VkImage _dst, uint32_t _width, uint32_t _height);
  void stage_copy(VkImage _src, VkImage _dst, const VkImageCopy *_info);
  void stage_copy(VkBuffer _src, VkImage _dst, const VkBufferImageCopy *_info);
  void stage_clear(VkImage _dst, VkClearColorValue *_clearColor);
  void destroy_vulkan();

  time_point next_job_time_point();

  void tick_posted(time_point _now);
  void tick_overridable(time_point _now);
  void tick_delayed(time_point _now);
  void jobs_loop();
  inline void check_thread() {
    assert(std::this_thread::get_id() == dispatcher_ || dispatcher_ == std::thread::id());
  }

#if defined(VK_USE_PLATFORM_XCB_KHR)
  xcb_connection_t *connection_;
  xcb_screen_t *screen_;
  xcb_key_symbols_t *keysyms_;
  std::unordered_map<xcb_window_t, window *> windows_;

  xcb_atom_t atom_wm_, atom_close_;
#endif
};

}  // namespace hut
