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
#include "hut/buffer_pool.hpp"

using namespace hut;

#ifdef HUT_ENABLE_VALIDATION
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT /*flags*/,
                                                     VkDebugReportObjectTypeEXT /*objType*/, uint64_t /*obj*/,
                                                     size_t /*location*/, int32_t /*code*/, const char *layerPrefix,
                                                     const char *msg, void * /*userData*/) {
  std::cout << '[' << layerPrefix << "] " << msg << std::endl;

  return VK_FALSE;
}
#endif

void display::post(display::callback _callback) {
  {
    std::lock_guard lock(posted_mutex_);
    posted_jobs_.emplace(posted_jobs_.end(), _callback);
  }
  post_empty_event();
}

void display::process_posts(time_point _now) {
  decltype(posted_jobs_) tmp;
  {
    std::lock_guard lock(posted_mutex_);
    tmp.swap(posted_jobs_);
  }

  for (const auto &job : tmp)
    job(_now);
}

shared_buffer display::alloc_buffer(uint _byte_size, VkMemoryPropertyFlags _type, VkBufferUsageFlagBits _flags) {
  return std::make_shared<buffer_pool>(*this, _byte_size, _type, _flags);
}

struct score_t {
  constexpr static uint32_t bad_id = std::numeric_limits<uint32_t>::max();
  uint32_t iqueueg_ = bad_id, iqueuec_ = bad_id, iqueuet_ = bad_id, iqueuep_ = bad_id;
  uint score_ = 0;
};

score_t rate_p_device(VkPhysicalDevice _device, VkSurfaceKHR _dummy) {
  score_t result{};

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceProperties(_device, &properties);
  vkGetPhysicalDeviceFeatures(_device, &features);

  result.score_ = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1000 : 100;
#ifdef HUT_PREFER_NONDESCRETE_DEVICES
  if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    result.score_ = 0;
#endif
  result.score_ += properties.limits.maxImageDimension2D;

  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(_device, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(_device, nullptr, &extension_count, available_extensions.data());
  bool has_swapchhain_ext = false;
  for (const auto &extension : available_extensions) {
    if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
      has_swapchhain_ext = true;
  }

  if (has_swapchhain_ext) {
    uint32_t famillies_count;
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &famillies_count, nullptr);
    std::vector<VkQueueFamilyProperties> famillies(famillies_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &famillies_count, famillies.data());

    for (uint32_t i = 0; i < famillies_count; i++) {
      const VkQueueFamilyProperties &props = famillies[i];
      VkBool32 present;
      vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, _dummy, &present);

      if (props.queueCount > 0) {
        // prioritise dedicated queues
        if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT && (props.queueFlags & (uint)~VK_QUEUE_GRAPHICS_BIT) == 0)
          result.iqueueg_ = i;
        else if (props.queueFlags & VK_QUEUE_COMPUTE_BIT && (props.queueFlags & (uint)~VK_QUEUE_COMPUTE_BIT) == 0)
          result.iqueuec_ = i;
        else if (props.queueFlags & VK_QUEUE_TRANSFER_BIT && (props.queueFlags & (uint)~VK_QUEUE_TRANSFER_BIT) == 0)
          result.iqueuet_ = i;

        if (result.iqueueg_ == score_t::bad_id && props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
          result.iqueueg_ = i;
        if (result.iqueuec_ == score_t::bad_id && props.queueFlags & VK_QUEUE_COMPUTE_BIT)
          result.iqueuec_ = i;
        if (result.iqueuet_ == score_t::bad_id && props.queueFlags & VK_QUEUE_TRANSFER_BIT)
          result.iqueuet_ = i;
        if (result.iqueuep_ == score_t::bad_id && present)
          result.iqueuep_ = i;
      }
    }

    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, _dummy, &capabilities);

    uint32_t formats_count, modes_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _dummy, &formats_count, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _dummy, &modes_count, nullptr);

    if (formats_count == 0)
      result.score_ = 0;
    if (modes_count == 0)
      result.score_ = 0;
  } else {
    result.score_ = 0;
  }

  if (result.iqueueg_ == score_t::bad_id)
    result.score_ = 0;
  if (result.iqueuec_ == score_t::bad_id)
    result.score_ = 0;
  if (result.iqueuet_ == score_t::bad_id)
    result.score_ = 0;
  if (result.iqueuep_ == score_t::bad_id)
    result.score_ = 0;

  return result;
}

const std::vector<const char *> layers = {
#ifdef HUT_ENABLE_VALIDATION
    "VK_LAYER_LUNARG_standard_validation",
#endif
};

void display::init_vulkan_instance(const char *_app_name, uint32_t _app_version,
                                   std::vector<const char *> &extensions) {
  extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

  uint32_t extension_count;
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());
#ifdef HUT_ENABLE_VALIDATION
  bool has_debug_ext = false;
  for (const auto &extension : available_extensions) {
    if (strcmp(extension.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
      has_debug_ext = true;
  }

  if (has_debug_ext)
    extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

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
    throw std::runtime_error(sstream("Couldn't create a vulkan instance, code: ") << result);

#ifdef HUT_ENABLE_VALIDATION
  if (has_debug_ext) {
    VkDebugReportCallbackCreateInfoEXT dinfo = {};
    dinfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    dinfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
        | VK_DEBUG_REPORT_ERROR_BIT_EXT
#ifdef HUT_ENABLE_VALIDATION_DEBUG
        | VK_DEBUG_REPORT_INFORMATION_BIT_EXT
#endif
        ;
    dinfo.pfnCallback = debug_callback;

    auto func = get_proc<PFN_vkCreateDebugReportCallbackEXT>("vkCreateDebugReportCallbackEXT");
    if (func != nullptr)
      func(instance_, &dinfo, nullptr, &debug_cb_);
  }
#endif
}

void display::init_vulkan_device(VkSurfaceKHR _dummy) {
  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
  std::vector<VkPhysicalDevice> physical_devices(device_count);
  vkEnumeratePhysicalDevices(instance_, &device_count, physical_devices.data());

  VkPhysicalDevice prefered_device = VK_NULL_HANDLE;
  score_t prefered_rate;
  for (VkPhysicalDevice &device : physical_devices) {
    score_t rate = rate_p_device(device, _dummy);
    if (rate.score_ > prefered_rate.score_) {
      prefered_rate = rate;
      prefered_device = device;
    }
  }
  if (prefered_device == VK_NULL_HANDLE)
    throw std::runtime_error("No suitable vulkan devices");

  vkGetPhysicalDeviceProperties(prefered_device, &device_props_);
  vkGetPhysicalDeviceFeatures(prefered_device, &device_features_);

  std::cout << "[hut] selected device " << device_props_.deviceName << " using Vulkan "
    << VK_VERSION_MAJOR(device_props_.apiVersion) << '.'
    << VK_VERSION_MINOR(device_props_.apiVersion) << std::endl;

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

  VkPhysicalDeviceFeatures enabled_features = {};
  enabled_features.samplerAnisotropy = device_features_.samplerAnisotropy;

  VkDeviceCreateInfo device_info = {};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.pQueueCreateInfos = queue_create_infos.data();
  device_info.queueCreateInfoCount = (uint32_t)queue_create_infos.size();
  device_info.pEnabledFeatures = &enabled_features;
  device_info.enabledExtensionCount = (uint32_t)device_extensions.size();
  device_info.ppEnabledExtensionNames = device_extensions.data();
  device_info.enabledLayerCount = (uint32_t)layers.size();
  device_info.ppEnabledLayerNames = layers.data();

  VkResult result = vkCreateDevice(pdevice_, &device_info, nullptr, &device_);
  if (result != VK_SUCCESS)
    throw std::runtime_error(sstream("Couldn't create a vulkan device, code: ") << result);

  vkGetDeviceQueue(device_, prefered_rate.iqueueg_, 0, &queueg_);
  vkGetDeviceQueue(device_, prefered_rate.iqueuec_, 0, &queuec_);
  vkGetDeviceQueue(device_, prefered_rate.iqueuet_, 0, &queuet_);
  vkGetDeviceQueue(device_, prefered_rate.iqueuep_, 0, &queuep_);

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = prefered_rate.iqueueg_;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandg_pool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }

  staging_ = std::make_shared<buffer_pool>(*this, 1024 * 1024,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandg_pool_;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(device_, &allocInfo, &staging_cb_);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(staging_cb_, &beginInfo);
}

void display::destroy_vulkan() {
  vkDeviceWaitIdle(device_);

  staging_.reset();
  vkFreeCommandBuffers(device_, commandg_pool_, 1, &staging_cb_);

  if (commandg_pool_ != VK_NULL_HANDLE)
    vkDestroyCommandPool(device_, commandg_pool_, nullptr);

  if (device_ != VK_NULL_HANDLE)
    vkDestroyDevice(device_, nullptr);

  if (debug_cb_ != VK_NULL_HANDLE) {
    auto func = get_proc<PFN_vkDestroyDebugReportCallbackEXT>("vkDestroyDebugReportCallbackEXT");
    if (func != nullptr)
      func(instance_, debug_cb_, nullptr);
  }

  if (instance_ != VK_NULL_HANDLE)
    vkDestroyInstance(instance_, nullptr);
}

std::pair<uint32_t, VkMemoryPropertyFlags> display::find_memory_type(uint32_t typeFilter,
                                                                     VkMemoryPropertyFlags properties) {
  // check_thread();

  for (uint32_t i = 0; i < mem_props_.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (mem_props_.memoryTypes[i].propertyFlags & properties) == properties) {
      return {i, mem_props_.memoryTypes[i].propertyFlags};
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void display::stage_transition(const image_transition &_info) {
#ifdef HUT_DEBUG_STAGING
  /*std::cout << "[staging] transition " << _info.destination << " from " << _info.oldLayout << " to "
            <<  _info.new_layout << std::endl;*/
#endif

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = _info.oldLayout;
  barrier.newLayout = _info.newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = _info.destination;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlagBits srcStage, dstStage;

  if (_info.oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && _info.newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_HOST_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (_info.oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && _info.newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    srcStage = VK_PIPELINE_STAGE_HOST_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (_info.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             _info.newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (_info.oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED &&
             _info.newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_HOST_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (_info.oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             _info.newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dstStage = VK_PIPELINE_STAGE_HOST_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(staging_cb_, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void display::stage_copy(const buffer_copy &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] buffer " << _info.source << '[' << _info.srcOffset << "-" << (_info.srcOffset+_info.size)
            << "] to " << _info.destination << '[' << _info.dstOffset << "-" << (_info.dstOffset+_info.size)
            << ']' << std::endl;
#endif

  vkCmdCopyBuffer(staging_cb_, _info.source, _info.destination, 1, &_info);
}

void display::stage_copy(const image_copy &_info) {
  vkCmdCopyImage(staging_cb_, _info.source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 _info.destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &_info);
}

void display::stage_copy(const buffer2image_copy &_info) {
#ifdef HUT_DEBUG_STAGING
  uint size = _info.bufferRowLength * _info.bufferImageHeight * _info.pixelSize;
  std::cout << "[staging] buffer " << _info.source << '[' << _info.bufferOffset << "-" << (_info.bufferOffset+size)
            << "] to image " << _info.destination << std::endl;
#endif

  vkCmdCopyBufferToImage(staging_cb_, _info.source, _info.destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &_info);
}

void display::stage_clear(const image_clear &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] clear image " << _info.destination << std::endl;
#endif

  VkImageSubresourceRange range = {};
  range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  range.baseArrayLayer = 0;
  range.layerCount = 1;
  range.baseMipLevel = 0;
  range.levelCount = 1;

  vkCmdClearColorImage(staging_cb_, _info.destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &_info.color, 1, &range);
}

void display::flush_staged() {
  std::lock_guard lk(staging_mutex_);
  if (staging_jobs_ == 0)
    return;

#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] doing preflush" << std::endl;
#endif
  for (auto &preflush : preflush_jobs_)
    preflush();
  preflush_jobs_.clear();

  if (staging_jobs_ > 0)
    return;

  vkEndCommandBuffer(staging_cb_);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &staging_cb_;

#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] submitting!" << std::endl;
#endif
  vkQueueSubmit(queueg_, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queueg_);

  staging_->clear_ranges();

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(staging_cb_, &beginInfo);
}
