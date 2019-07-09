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

#include "hut/utils.hpp"

namespace hut {

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

  inline void postflush(flush_callback _callback) {
    postflush_jobs_.emplace_back(_callback);
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
  std::map<size_t, callback> overridable_jobs_;
  std::multimap<time_point, callback> delayed_jobs_;
  std::mutex posted_mutex_, overridable_mutex_, delayed_mutex_;
  std::mutex cv_mutex_;
  std::condition_variable cv_;
  std::thread::id dispatcher_;

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
