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

#include <cassert>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <set>
#include <unordered_set>

#include "hut/display.hpp"

using namespace hut;

void display::post(display::callback _callback) {
  {
    std::lock_guard<std::mutex> lock(posted_mutex_);
    posted_jobs_.emplace(posted_jobs_.end(), _callback);
  }
  cv_.notify_one();
}

void display::post_overridable(callback _callback, size_t _id) {
  {
    std::lock_guard<std::mutex> lock(overridable_mutex_);
    overridable_jobs_.emplace(_id, _callback);
  }
  cv_.notify_one();
}

void display::post_delayed(display::callback _callback,
                           std::chrono::milliseconds _delay) {
  {
    std::lock_guard<std::mutex> lock(delayed_mutex_);
    delayed_jobs_.emplace(std::chrono::steady_clock::now() + _delay, _callback);
  }
  cv_.notify_one();
}

display::time_point display::next_job_time_point() {
  time_point result = time_point::max();
  {
    std::lock_guard<std::mutex> lock(overridable_mutex_);
    if (!overridable_jobs_.empty()) return time_point::min();
  }
  {
    std::lock_guard<std::mutex> lock(posted_mutex_);
    if (!posted_jobs_.empty()) return time_point::min();
  }
  {
    std::lock_guard<std::mutex> lock(delayed_mutex_);
    if (!delayed_jobs_.empty())
      result = std::min(result, delayed_jobs_.begin()->first);
  }
  return result;
}

void display::tick_posted(display::time_point _now) {
  decltype(posted_jobs_) tmp;
  {
    std::lock_guard<std::mutex> lock(posted_mutex_);
    tmp.swap(posted_jobs_);
  }

  for (const auto &job : tmp) job(_now);
}

void display::tick_overridable(display::time_point _now) {
  decltype(overridable_jobs_) tmp;
  {
    std::lock_guard<std::mutex> lock(overridable_mutex_);
    tmp.swap(overridable_jobs_);
  }

  for (const auto &job : tmp) job.second(_now);
}

void display::tick_delayed(display::time_point _now) {
  decltype(delayed_jobs_) tmp;
  {
    std::lock_guard<std::mutex> lock(delayed_mutex_);
    tmp.swap(delayed_jobs_);
  }

  for (auto it = tmp.begin(); it != tmp.end();) {
    if (it->first > _now) break;
    it->second(_now);
    it = tmp.erase(it);
  }

  {
    std::lock_guard<std::mutex> lock(delayed_mutex_);
    for (const auto &due : tmp) delayed_jobs_.insert(due);
  }
}

void display::jobs_loop() {
  time_point next = next_job_time_point();
  if (next == time_point::max()) {
    std::unique_lock<std::mutex> lk(cv_mutex_);
    cv_.wait(lk);
  } else if (next != time_point::min()) {
    std::unique_lock<std::mutex> lk(cv_mutex_);
    std::chrono::duration<double, std::milli> wait =
        next - std::chrono::steady_clock::now();
    cv_.wait_until(lk, next);
  }

  const time_point now = std::chrono::steady_clock::now();
  tick_overridable(now);
  tick_posted(now);
  tick_delayed(now);
  std::chrono::duration<double, std::milli> duration =
      std::chrono::steady_clock::now() - now;
  // if (duration > std::chrono::milliseconds(16))
  // std::cout << "Jobs done in " << duration.count() << "ms" << std::endl;
}

struct score_t {
  constexpr static uint32_t bad_id = std::numeric_limits<uint32_t>::max();
  uint32_t iqueueg_ = bad_id, iqueuec_ = bad_id, iqueuet_ = bad_id,
           iqueuep_ = bad_id;
  uint score_ = 0;
};

score_t rate_p_device(VkPhysicalDevice _device, VkSurfaceKHR _dummy) {
  score_t result{};

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceProperties(_device, &properties);
  vkGetPhysicalDeviceFeatures(_device, &features);

  result.score_ = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                      ? 1000
                      : 100;
  result.score_ += properties.limits.maxImageDimension2D;

  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(_device, nullptr, &extension_count,
                                       nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(_device, nullptr, &extension_count,
                                       available_extensions.data());
  std::cout << "\tAvailable device extension:" << std::endl;
  bool has_swapchhain_ext = false;
  for (const auto &extension : available_extensions) {
    std::cout << "\t\t" << extension.extensionName << std::endl;
    if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
      has_swapchhain_ext = true;
  }

  if (has_swapchhain_ext) {
    uint32_t famillies_count;
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &famillies_count,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> famillies(famillies_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &famillies_count,
                                             famillies.data());

    std::cout << "\tAvailable queue famillies:" << std::endl;
    for (uint32_t i = 0; i < famillies_count; i++) {
      const VkQueueFamilyProperties &props = famillies[i];
      VkBool32 present;
      vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, _dummy, &present);

      std::cout << "\t\tcount:" << props.queueCount
                << ", flags: " << props.queueFlags << ", present: " << present
                << std::endl;

      if (props.queueCount > 0) {
        // prioritise dedicated queues
        if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            (props.queueFlags & (uint)~VK_QUEUE_GRAPHICS_BIT) == 0)
          result.iqueueg_ = i;
        else if (props.queueFlags & VK_QUEUE_COMPUTE_BIT &&
                 (props.queueFlags & (uint)~VK_QUEUE_COMPUTE_BIT) == 0)
          result.iqueuec_ = i;
        else if (props.queueFlags & VK_QUEUE_TRANSFER_BIT &&
                 (props.queueFlags & (uint)~VK_QUEUE_TRANSFER_BIT) == 0)
          result.iqueuet_ = i;

        if (result.iqueueg_ == score_t::bad_id &&
            props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
          result.iqueueg_ = i;
        if (result.iqueuec_ == score_t::bad_id &&
            props.queueFlags & VK_QUEUE_COMPUTE_BIT)
          result.iqueuec_ = i;
        if (result.iqueuet_ == score_t::bad_id &&
            props.queueFlags & VK_QUEUE_TRANSFER_BIT)
          result.iqueuet_ = i;
        if (result.iqueuep_ == score_t::bad_id && present) result.iqueuep_ = i;
      }
    }

    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, _dummy, &capabilities);

    uint32_t formats_count, modes_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _dummy, &formats_count,
                                         nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _dummy, &modes_count,
                                              nullptr);

    if (formats_count == 0) {
      std::cout << "\teliminated, no formats" << std::endl;
      result.score_ = 0;
    }
    if (modes_count == 0) {
      std::cout << "\teliminated, no present modes" << std::endl;
      result.score_ = 0;
    }
  } else {
    std::cout << "\teliminated, no swapchain support" << std::endl;
    result.score_ = 0;
  }

  if (result.iqueueg_ == score_t::bad_id) {
    std::cout << "\teliminated, no graphics support" << std::endl;
    result.score_ = 0;
  }
  if (result.iqueuec_ == score_t::bad_id) {
    std::cout << "\teliminated, no compute support" << std::endl;
    result.score_ = 0;
  }
  if (result.iqueuet_ == score_t::bad_id) {
    std::cout << "\teliminated, no transfer support" << std::endl;
    result.score_ = 0;
  }
  if (result.iqueuep_ == score_t::bad_id) {
    std::cout << "\teliminated, no present support" << std::endl;
    result.score_ = 0;
  }

  return result;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugReportFlagsEXT /*flags*/, VkDebugReportObjectTypeEXT /*objType*/,
    uint64_t /*obj*/, size_t /*location*/, int32_t /*code*/,
    const char *layerPrefix, const char *msg, void * /*userData*/) {
  std::cout << '[' << layerPrefix << "] " << msg << std::endl;

  return VK_FALSE;
}

const std::vector<const char *> layers = {
#ifndef NDEBUG
    "VK_LAYER_LUNARG_standard_validation",
#endif
};

void display::init_vulkan_instance(const char *_app_name, uint32_t _app_version,
                                   std::vector<const char *> &extensions) {
  extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

  uint32_t extension_count;
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                         available_extensions.data());
  std::cout << "Available vulkan extension:" << std::endl;
  bool has_debug_ext = false;
  for (const auto &extension : available_extensions) {
    std::cout << '\t' << extension.extensionName << std::endl;
#ifndef NDEBUG
    if (strcmp(extension.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) ==
        0)
      has_debug_ext = true;
#endif
  }

  if (has_debug_ext)
    extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.apiVersion = VK_API_VERSION_1_0;
  appInfo.pEngineName = "hut";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pApplicationName = _app_name;
  appInfo.applicationVersion = _app_version;

  VkInstanceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.enabledExtensionCount = (uint32_t)extensions.size();
  info.ppEnabledExtensionNames = extensions.data();
  info.enabledLayerCount = (uint32_t)layers.size();
  info.ppEnabledLayerNames = layers.data();
  info.pApplicationInfo = &appInfo;

  VkResult result = vkCreateInstance(&info, nullptr, &instance_);
  if (result != VK_SUCCESS)
    throw std::runtime_error(
        sstream("Couldn't create a vulkan instance, code: ") << result);

  if (has_debug_ext) {
    VkDebugReportCallbackCreateInfoEXT dinfo = {};
    dinfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    dinfo.flags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
#ifndef NDEBUG
    dinfo.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
#endif
    dinfo.pfnCallback = debug_callback;

    auto func = get_proc<PFN_vkCreateDebugReportCallbackEXT>(
        "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) func(instance_, &dinfo, nullptr, &debug_cb_);
  }
}

void display::init_vulkan_device(VkSurfaceKHR _dummy) {
  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
  std::vector<VkPhysicalDevice> physical_devices(device_count);
  vkEnumeratePhysicalDevices(instance_, &device_count, physical_devices.data());

  VkPhysicalDevice prefered_device = VK_NULL_HANDLE;
  score_t prefered_rate;
  std::cout << "Available vulkan devices:" << std::endl;
  for (VkPhysicalDevice &device : physical_devices) {
    score_t rate = rate_p_device(device, _dummy);
    std::cout << '\t' << device << ", scored: " << rate.score_ << std::endl;
    if (rate.score_ > prefered_rate.score_) {
      prefered_rate = rate;
      prefered_device = device;
    }
  }
  if (prefered_device == VK_NULL_HANDLE)
    throw std::runtime_error("No suitable vulkan devices");

  pdevice_ = prefered_device;
  iqueueg_ = prefered_rate.iqueueg_;
  iqueuec_ = prefered_rate.iqueuec_;
  iqueuet_ = prefered_rate.iqueuet_;
  iqueuep_ = prefered_rate.iqueuep_;

  vkGetPhysicalDeviceMemoryProperties(pdevice_, &mem_props_);

  std::unordered_set<uint32_t> unique_indexes;
  unique_indexes.emplace(prefered_rate.iqueueg_);
  unique_indexes.emplace(prefered_rate.iqueuec_);
  unique_indexes.emplace(prefered_rate.iqueuet_);
  unique_indexes.emplace(prefered_rate.iqueuep_);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queue_create_info = {};
  queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueCount = 1;
  queue_create_info.pQueuePriorities = &queuePriority;
  for (uint32_t index : unique_indexes) {
    queue_create_info.queueFamilyIndex = index;
    queue_create_infos.emplace_back(queue_create_info);
  }

  const std::vector<const char *> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  VkPhysicalDeviceFeatures device_features = {};
  VkDeviceCreateInfo device_info = {};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.pQueueCreateInfos = queue_create_infos.data();
  device_info.queueCreateInfoCount = (uint32_t)queue_create_infos.size();
  device_info.pEnabledFeatures = &device_features;
  device_info.enabledExtensionCount = (uint32_t)device_extensions.size();
  device_info.ppEnabledExtensionNames = device_extensions.data();
  device_info.enabledLayerCount = (uint32_t)layers.size();
  device_info.ppEnabledLayerNames = layers.data();

  VkResult result = vkCreateDevice(pdevice_, &device_info, nullptr, &device_);
  if (result != VK_SUCCESS)
    throw std::runtime_error(sstream("Couldn't create a vulkan device, code: ")
                             << result);

  vkGetDeviceQueue(device_, prefered_rate.iqueueg_, 0, &queueg_);
  vkGetDeviceQueue(device_, prefered_rate.iqueuec_, 0, &queuec_);
  vkGetDeviceQueue(device_, prefered_rate.iqueuet_, 0, &queuet_);
  vkGetDeviceQueue(device_, prefered_rate.iqueuep_, 0, &queuep_);

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = prefered_rate.iqueueg_;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandg_pool_) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

void display::destroy_vulkan() {
  vkDeviceWaitIdle(device_);

  if (commandg_pool_ != VK_NULL_HANDLE)
    vkDestroyCommandPool(device_, commandg_pool_, nullptr);

  if (device_ != VK_NULL_HANDLE) vkDestroyDevice(device_, nullptr);

  if (debug_cb_ != VK_NULL_HANDLE) {
    auto func = get_proc<PFN_vkDestroyDebugReportCallbackEXT>(
        "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) func(instance_, debug_cb_, nullptr);
  }

  if (instance_ != VK_NULL_HANDLE) vkDestroyInstance(instance_, nullptr);
}

uint32_t display::find_memory_type(uint32_t typeFilter,
                                   VkMemoryPropertyFlags properties) {
  check_thread();

  for (uint32_t i = 0; i < mem_props_.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (mem_props_.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void display::check_thread() {
  assert(std::this_thread::get_id() == dispatcher_ ||
         dispatcher_ == std::thread::id());
}
