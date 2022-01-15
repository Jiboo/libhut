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

#include "hut/display.hpp"

#include <cstring>

#include <algorithm>
#include <iostream>
#include <set>
#include <unordered_set>

#include "hut/utils/profiling.hpp"

#include "hut/buffer.hpp"
#include "hut/window.hpp"

using namespace hut;

#ifdef HUT_ENABLE_VALIDATION
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags,
                                                     VkDebugReportObjectTypeEXT /*objType*/, u64 /*obj*/,
                                                     size_t /*location*/, i32 /*code*/, const char *layerPrefix,
                                                     const char *msg, void * /*userData*/) {
  if (strcmp(layerPrefix, "Loader Message") == 0)
    return VK_FALSE;
  if (strcmp(layerPrefix, "radv") == 0)
    return VK_FALSE;

  char level;
  switch (flags) {
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT: level = 'I'; break;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT: level = 'W'; break;
    case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT: level = 'P'; break;
    case VK_DEBUG_REPORT_ERROR_BIT_EXT: level = 'E'; break;
    default: level = 'D'; break;
  }

  std::cout << '[' << level << ' ' << layerPrefix << "] " << msg << std::endl;
#  if defined(HUT_ENABLE_VALIDATION_DEBUG) && defined(HUT_ENABLE_PROFILING)
  boost::stacktrace::stacktrace st;
  if (st) {
    for (const auto &frame : st.as_vector()) {
      std::cout << "\t[" << layerPrefix << "] callstack: " << frame << std::endl;
    }
  }
#  endif  //HUT_ENABLE_VALIDATION_DEBUG
  return VK_FALSE;
}
#endif  //HUT_ENABLE_VALIDATION

const char *hut::keysym_name(keysym _keysym) {
  switch (_keysym) {
#define HUT_MAP_KEYSYM(FORMAT_LINUX, FORMAT_X11, FORMAT_IMGUI)                                                         \
  case KSYM_##FORMAT_LINUX:                                                                                            \
    return "KSYM_" #FORMAT_LINUX;
#include "hut/keysyms.inc"
#undef HUT_MAP_KEYSYM
    default: return "KSYM_INVALID";
  }
}

const char *hut::cursor_css_name(cursor_type _c) {
  using namespace hut;
  switch (_c) {
    case CNONE: return "none";
    default: [[fallthrough]];
    case CDEFAULT: return "default";
    case CCONTEXT_MENU: return "context-menu";
    case CHELP: return "help";
    case CPOINTER: return "pointer";
    case CPROGRESS: return "progress";
    case CWAIT: return "wait";
    case CCELL: return "cell";
    case CCROSSHAIR: return "crosshair";
    case CTEXT: return "text";
    case CALIAS: return "alias";
    case CCOPY: return "copy";
    case CMOVE: return "move";
    case CNO_DROP: return "no-drop";
    case CNOT_ALLOWED: return "not-allowed";
    case CGRAB: return "grab";
    case CGRABBING: return "grabbing";
    case CSCROLL_ALL: return "all-scroll";
    case CRESIZE_COL: return "col-resize";
    case CRESIZE_ROW: return "row-resize";
    case CRESIZE_N: return "n-resize";
    case CRESIZE_E: return "e-resize";
    case CRESIZE_S: return "s-resize";
    case CRESIZE_W: return "w-resize";
    case CRESIZE_NE: return "ne-resize";
    case CRESIZE_NW: return "nw-resize";
    case CRESIZE_SE: return "se-resize";
    case CRESIZE_SW: return "sw-resize";
    case CRESIZE_EW: return "ew-resize";
    case CRESIZE_NS: return "ns-resize";
    case CRESIZE_NESW: return "nesw-resize";
    case CRESIZE_NWSE: return "nwse-resize";
    case CZOOM_IN: return "zoom-in";
    case CZOOM_OUT: return "zoom-out";
  }
}

const char *hut::format_mime_type(clipboard_format _f) {
  switch (_f) {
    case FTEXT_PLAIN: return "text/plain;charset=utf-8";
    case FTEXT_HTML: return "text/html";
    case FTEXT_URI_LIST: return "text/uri-list";
    case FIMAGE_BMP: return "image/bmp";
    case FIMAGE_PNG: return "image/png";
    case FIMAGE_JPEG: return "image/jpeg";
  }
  return nullptr;
}

std::optional<clipboard_format> hut::mime_type_format(const char *_mime_type) {
  if (strcmp(_mime_type, "text/plain;charset=utf-8") == 0)
    return FTEXT_PLAIN;
  else if (strcmp(_mime_type, "text/html") == 0)
    return FTEXT_HTML;
  else if (strcmp(_mime_type, "text/uri-list") == 0)
    return FTEXT_URI_LIST;
  else if (strcmp(_mime_type, "image/bmp") == 0)
    return FIMAGE_BMP;
  else if (strcmp(_mime_type, "image/png") == 0)
    return FIMAGE_PNG;
  else if (strcmp(_mime_type, "image/jpeg") == 0)
    return FIMAGE_JPEG;
  return {};
}

const char *hut::action_name(dragndrop_action _a) {
  switch (_a) {
    case DNDNONE: return "None";
    case DNDMOVE: return "Move";
    case DNDCOPY: return "Copy";
  }
  return nullptr;
}

const char *hut::modifier_name(modifier _m) {
  switch (_m) {
    case KMOD_CTRL: return "Ctrl";
    case KMOD_ALT: return "Alt";
    case KMOD_SHIFT: return "Shift";
  }
  return nullptr;
}

void display::post(const display::callback &_callback) {
  {
    std::lock_guard lock(posted_mutex_);
    posted_jobs_.emplace(posted_jobs_.end(), _callback);
  }
  post_empty_event();
}

void display::process_posts(time_point _now) {
  HUT_PROFILE_SCOPE(PDISPLAY, "display::process_posts {}", posted_jobs_.size());
  decltype(posted_jobs_) tmp;
  {
    std::lock_guard lock(posted_mutex_);
    tmp.swap(posted_jobs_);
  }

  for (const auto &job : tmp)
    job(_now);
}

struct score_t {
  constexpr static u32 bad_id   = std::numeric_limits<u32>::max();
  u32                  iqueueg_ = bad_id, iqueuec_ = bad_id, iqueuet_ = bad_id, iqueuep_ = bad_id;
  uint                 score_ = 0;
};

score_t rate_p_device(VkPhysicalDevice _device, VkSurfaceKHR _dummy) {
  score_t result{};
  result.score_ = 1;

  VkPhysicalDeviceVulkan12Properties properties12{};
  properties12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
  VkPhysicalDeviceProperties2 properties2;
  properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  properties2.pNext = &properties12;
  HUT_PVK(vkGetPhysicalDeviceProperties2, _device, &properties2);
  VkPhysicalDeviceProperties &properties = properties2.properties;

#ifdef HUT_PREFER_NONDESCRETE_DEVICES
  if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    result.score_ = 0;
    return result;
  }
#endif

  VkPhysicalDeviceVulkan12Features features12{};
  features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  VkPhysicalDeviceFeatures2 features2{};
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2.pNext = &features12;
  HUT_PVK(vkGetPhysicalDeviceFeatures2, _device, &features2);
  VkPhysicalDeviceFeatures &features = features2.features;

  if (features12.descriptorBindingPartiallyBound == VK_FALSE)
    result.score_ = 0;
  if (features12.shaderSampledImageArrayNonUniformIndexing == VK_FALSE)
    result.score_ = 0;
  if (features2.features.multiDrawIndirect == VK_FALSE)
    result.score_ = 0;

#ifdef HUT_ENABLE_VALIDATION_DEBUG
  std::cout << "[hut] rating device " << properties.deviceName << " using Vulkan "
            << VK_VERSION_MAJOR(properties.apiVersion) << '.' << VK_VERSION_MINOR(properties.apiVersion) << ", "
            << (result.score_ == 0 ? "don't" : "do") << " have minimum features." << std::endl;
#endif

  if (result.score_ == 0)
    return result;

  u32 extension_count;
  HUT_PVK(vkEnumerateDeviceExtensionProperties, _device, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  HUT_PVK(vkEnumerateDeviceExtensionProperties, _device, nullptr, &extension_count, available_extensions.data());
  bool has_swapchhain_ext = false;
  for (const auto &extension : available_extensions) {
    if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
      has_swapchhain_ext = true;
  }

  if (_dummy && !has_swapchhain_ext)
    return result;

  if (has_swapchhain_ext) {
    u32 famillies_count;
    HUT_PVK(vkGetPhysicalDeviceQueueFamilyProperties, _device, &famillies_count, nullptr);
    std::vector<VkQueueFamilyProperties> famillies(famillies_count);
    HUT_PVK(vkGetPhysicalDeviceQueueFamilyProperties, _device, &famillies_count, famillies.data());

    for (u32 i = 0; i < famillies_count; i++) {
      const VkQueueFamilyProperties &props = famillies[i];

      if (_dummy) {
        VkBool32 present;
        HUT_PVK(vkGetPhysicalDeviceSurfaceSupportKHR, _device, i, _dummy, &present);

        VkSurfaceCapabilitiesKHR capabilities = {};
        HUT_PVK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, _device, _dummy, &capabilities);

        u32 formats_count, modes_count;
        HUT_PVK(vkGetPhysicalDeviceSurfaceFormatsKHR, _device, _dummy, &formats_count, nullptr);
        HUT_PVK(vkGetPhysicalDeviceSurfacePresentModesKHR, _device, _dummy, &modes_count, nullptr);

        if (props.queueCount > 0) {
          // prioritise dedicated queues
          if (result.iqueuep_ == score_t::bad_id && present)
            result.iqueuep_ = i;
        }

        if (formats_count == 0)
          result.score_ = 0;
        if (modes_count == 0)
          result.score_ = 0;
      }

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
      }
    }

    if (result.iqueueg_ == score_t::bad_id)
      result.score_ = 0;
    if (result.iqueuec_ == score_t::bad_id)
      result.score_ = 0;
    if (result.iqueuet_ == score_t::bad_id)
      result.score_ = 0;
    if (_dummy && result.iqueuep_ == score_t::bad_id)
      result.score_ = 0;
  }

  result.score_ += properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1000 : 100;
  result.score_ += properties.limits.maxImageDimension2D;

#ifdef HUT_ENABLE_VALIDATION_DEBUG
  std::cout << "[hut] device " << properties.deviceName << " using Vulkan " << VK_VERSION_MAJOR(properties.apiVersion)
            << '.' << VK_VERSION_MINOR(properties.apiVersion) << " scored " << result.score_ << std::endl;
#endif

  return result;
}

const std::vector<const char *> layers = {
#ifdef HUT_ENABLE_VALIDATION
    "VK_LAYER_KHRONOS_validation",
#endif
};

void display::init_vulkan_instance(const char *_app_name, u32 _app_version, std::vector<const char *> &extensions) {
  HUT_PROFILE_SCOPE(PDISPLAY, "display::init_vulkan_instance");
  extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

  u32 extension_count;
  HUT_PVK(vkEnumerateInstanceExtensionProperties, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  HUT_PVK(vkEnumerateInstanceExtensionProperties, nullptr, &extension_count, available_extensions.data());

  for (const auto &extension : available_extensions) {
#ifdef HUT_ENABLE_VALIDATION
    if (strcmp(extension.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
      extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
  }

  VkApplicationInfo appInfo  = {};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.apiVersion         = VK_API_VERSION_1_2;
  appInfo.pEngineName        = "hut";
  appInfo.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pApplicationName   = _app_name;
  appInfo.applicationVersion = _app_version;

  VkInstanceCreateInfo info    = {};
  info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.enabledExtensionCount   = (u32)extensions.size();
  info.ppEnabledExtensionNames = extensions.data();
  info.enabledLayerCount       = (u32)layers.size();
  info.ppEnabledLayerNames     = layers.data();
  info.pApplicationInfo        = &appInfo;

  VkResult result = HUT_PVK(vkCreateInstance, &info, nullptr, &instance_);
  if (result != VK_SUCCESS)
    throw std::runtime_error(sstream("Couldn't create a vulkan instance, code: ") << result);

#ifdef HUT_ENABLE_VALIDATION
  if (std::find(extensions.cbegin(), extensions.cend(), VK_EXT_DEBUG_REPORT_EXTENSION_NAME) != extensions.cend()) {
    VkDebugReportCallbackCreateInfoEXT dinfo = {};
    dinfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    dinfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
                | VK_DEBUG_REPORT_ERROR_BIT_EXT
#  ifdef HUT_ENABLE_VALIDATION_DEBUG
                | VK_DEBUG_REPORT_INFORMATION_BIT_EXT
#  endif
        ;
    dinfo.pfnCallback = debug_callback;

    auto func = get_proc<PFN_vkCreateDebugReportCallbackEXT>("vkCreateDebugReportCallbackEXT");
    if (func != nullptr)
      func(instance_, &dinfo, nullptr, &debug_cb_);
  }
#endif
}

void display::init_vulkan_device(VkSurfaceKHR _dummy) {
  HUT_PROFILE_SCOPE(PDISPLAY, "display::init_vulkan_device");
  u32 device_count;
  HUT_PVK(vkEnumeratePhysicalDevices, instance_, &device_count, nullptr);
  std::vector<VkPhysicalDevice> physical_devices(device_count);
  HUT_PVK(vkEnumeratePhysicalDevices, instance_, &device_count, physical_devices.data());

  VkPhysicalDevice prefered_device = VK_NULL_HANDLE;
  score_t          prefered_rate;
  for (VkPhysicalDevice &device : physical_devices) {
    score_t rate = rate_p_device(device, _dummy);
    if (rate.score_ > 0 && rate.score_ > prefered_rate.score_) {
      prefered_rate   = rate;
      prefered_device = device;
    }
  }
  if (prefered_device == VK_NULL_HANDLE)
    throw std::runtime_error("No suitable vulkan devices");

  HUT_PVK(vkGetPhysicalDeviceProperties, prefered_device, &device_props_);
  HUT_PVK(vkGetPhysicalDeviceFeatures, prefered_device, &device_features_);

#ifdef HUT_ENABLE_VALIDATION_DEBUG
  std::cout << "[hut] selected device " << device_props_.deviceName << " using Vulkan "
            << VK_VERSION_MAJOR(device_props_.apiVersion) << '.' << VK_VERSION_MINOR(device_props_.apiVersion)
            << std::endl;
#endif

  pdevice_ = prefered_device;
  iqueueg_ = prefered_rate.iqueueg_;
  iqueuec_ = prefered_rate.iqueuec_;
  iqueuet_ = prefered_rate.iqueuet_;
  iqueuep_ = prefered_rate.iqueuep_;

  HUT_PVK(vkGetPhysicalDeviceMemoryProperties, pdevice_, &mem_props_);

  std::unordered_set<u32> unique_indexes;
  unique_indexes.emplace(prefered_rate.iqueueg_);
  unique_indexes.emplace(prefered_rate.iqueuec_);
  unique_indexes.emplace(prefered_rate.iqueuet_);
  if (prefered_rate.iqueuep_ != score_t::bad_id)
    unique_indexes.emplace(prefered_rate.iqueuep_);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  float                                queuePriority     = 1.0f;
  VkDeviceQueueCreateInfo              queue_create_info = {};
  queue_create_info.sType                                = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueCount                           = 1;
  queue_create_info.pQueuePriorities                     = &queuePriority;
  for (u32 index : unique_indexes) {
    queue_create_info.queueFamilyIndex = index;
    queue_create_infos.emplace_back(queue_create_info);
  }

  u32 extension_count;
  HUT_PVK(vkEnumerateDeviceExtensionProperties, prefered_device, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  HUT_PVK(vkEnumerateDeviceExtensionProperties, prefered_device, nullptr, &extension_count,
          available_extensions.data());

  std::vector<const char *> extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  for (const auto &extension : available_extensions) {
#ifdef HUT_ENABLE_PROFILING
    if (strcmp(extension.extensionName, VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME) == 0)
      extensions.emplace_back(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);
#endif
  }

  VkPhysicalDeviceFeatures core_features               = {};
  core_features.samplerAnisotropy                      = device_features_.samplerAnisotropy;
  core_features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
  core_features.multiDrawIndirect                      = VK_TRUE;
  //core_features.textureCompressionBC = VK_TRUE;
  //core_features.textureCompressionASTC_LDR = VK_TRUE;
  //core_features.textureCompressionETC2 = VK_TRUE;

  VkPhysicalDeviceDescriptorIndexingFeatures bindings_features = {};
  bindings_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
  bindings_features.shaderSampledImageArrayNonUniformIndexing     = VK_TRUE;
  bindings_features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
  bindings_features.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
  bindings_features.descriptorBindingPartiallyBound               = VK_TRUE;

  VkPhysicalDeviceFeatures2 device_features = {};
  device_features.sType                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  device_features.pNext                     = &bindings_features;
  device_features.features                  = core_features;

  VkDeviceCreateInfo device_info      = {};
  device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.pQueueCreateInfos       = queue_create_infos.data();
  device_info.queueCreateInfoCount    = (u32)queue_create_infos.size();
  device_info.pEnabledFeatures        = nullptr;
  device_info.pNext                   = &device_features;
  device_info.enabledExtensionCount   = (u32)extensions.size();
  device_info.ppEnabledExtensionNames = extensions.data();
  device_info.enabledLayerCount       = (u32)layers.size();
  device_info.ppEnabledLayerNames     = layers.data();

  VkResult result = HUT_PVK(vkCreateDevice, pdevice_, &device_info, nullptr, &device_);
  if (result != VK_SUCCESS)
    throw std::runtime_error(sstream("Couldn't create a vulkan device, code: ") << result);

  HUT_PVK(vkGetDeviceQueue, device_, prefered_rate.iqueueg_, 0, &queueg_);
  HUT_PVK(vkGetDeviceQueue, device_, prefered_rate.iqueuec_, 0, &queuec_);
  HUT_PVK(vkGetDeviceQueue, device_, prefered_rate.iqueuet_, 0, &queuet_);
  if (prefered_rate.iqueuep_ != score_t::bad_id)
    HUT_PVK(vkGetDeviceQueue, device_, prefered_rate.iqueuep_, 0, &queuep_);

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex        = prefered_rate.iqueueg_;
  poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (HUT_PVK(vkCreateCommandPool, device_, &poolInfo, nullptr, &commandg_pool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }

  buffer_params staging_params;
  staging_params.permanent_map_ = true;
  staging_params.type_          = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  staging_params.usage_ = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  staging_              = std::make_shared<buffer>(*this, 1024 * 1024, staging_params);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool                 = commandg_pool_;
  allocInfo.commandBufferCount          = 1;

  HUT_PVK(vkAllocateCommandBuffers, device_, &allocInfo, &staging_cb_);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  HUT_PVK(vkBeginCommandBuffer, staging_cb_, &beginInfo);
}

void display::destroy_vulkan() {
  HUT_PVK(vkDeviceWaitIdle, device_);

  staging_.reset();
  HUT_PVK(vkFreeCommandBuffers, device_, commandg_pool_, 1, &staging_cb_);

  if (commandg_pool_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroyCommandPool, device_, commandg_pool_, nullptr);

  if (device_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroyDevice, device_, nullptr);

  if (debug_cb_ != VK_NULL_HANDLE) {
    auto func = get_proc<PFN_vkDestroyDebugReportCallbackEXT>("vkDestroyDebugReportCallbackEXT");
    if (func != nullptr)
      func(instance_, debug_cb_, nullptr);
  }

  if (instance_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroyInstance, instance_, nullptr);
}

std::pair<u32, VkMemoryPropertyFlags> display::find_memory_type(u32 typeFilter, VkMemoryPropertyFlags properties) {
  for (u32 i = 0; i < mem_props_.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (mem_props_.memoryTypes[i].propertyFlags & properties) == properties) {
      return {i, mem_props_.memoryTypes[i].propertyFlags};
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

std::ostream &operator<<(std::ostream &_os, const VkImageLayout _layout) {
  switch (_layout) {
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return _os << "READ";
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return _os << "SRC";
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return _os << "DST";
    case VK_IMAGE_LAYOUT_PREINITIALIZED: return _os << "PREINIT";
    default: return _os << "???";
  }
}

std::ostream &operator<<(std::ostream &_os, const VkOffset3D _offset) {
  return _os << "offset " << ivec3{_offset.x, _offset.y, _offset.z};
}

std::ostream &operator<<(std::ostream &_os, const VkExtent3D _extent) {
  return _os << "extent " << uvec3{_extent.width, _extent.height, _extent.depth};
}

void display::transition_image(VkCommandBuffer _cb, VkImage _image, VkImageSubresourceRange _range,
                               VkImageLayout _oldLayout, VkImageLayout _newLayout) {
  VkImageMemoryBarrier barrier = {};
  barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout            = _oldLayout;
  barrier.newLayout            = _newLayout;
  barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                = _image;
  barrier.subresourceRange     = _range;

  VkPipelineStageFlagBits srcStage, dstStage;

  if (_oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && _newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    srcStage              = VK_PIPELINE_STAGE_HOST_BIT;
    dstStage              = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (_oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && _newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    srcStage              = VK_PIPELINE_STAGE_HOST_BIT;
    dstStage              = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (_oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && _newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage              = VK_PIPELINE_STAGE_HOST_BIT;
    dstStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else if (_oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             && _newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    srcStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dstStage              = VK_PIPELINE_STAGE_HOST_BIT;
  } else if (_oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
             && _newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage              = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else if (_oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             && _newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    srcStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dstStage              = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (_oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
             && _newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage              = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else if (_oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             && _newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    srcStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dstStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (_oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
             && _newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dstStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  HUT_PVK_NAMED_ALIASED(vkCmdPipelineBarrier, ("dest", "from", "to"), ((void *)_image, _oldLayout, _newLayout), _cb,
                        srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void display::postflush_collect(buffer_suballoc<u8> &&_callback) {
  postflush_garbage_.emplace_back(std::move(_callback));
}

void display::stage_transition(const image_transition &_info, VkImageSubresourceRange _range) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] transition " << _info.destination << " from " << _info.oldLayout << " to " << _info.newLayout
            << std::endl;
#endif

  transition_image(staging_cb_, _info.destination, _range, _info.oldLayout, _info.newLayout);
}

void display::stage_copy(const buffer_copy &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] buffer_copy " << _info.source << '[' << _info.srcOffset << "-"
            << (_info.srcOffset + _info.size) << "] to " << _info.destination << '[' << _info.dstOffset << "-"
            << (_info.dstOffset + _info.size) << ']' << std::endl;
#endif

  HUT_PVK_NAMED_ALIASED(vkCmdCopyBuffer, ("src", "srcOffset", "size", "dst", "dstOffset"),
                        ((void *)_info.source, _info.srcOffset, _info.size, (void *)_info.destination, _info.dstOffset),
                        staging_cb_, _info.source, _info.destination, 1, &_info);
}

void display::stage_zero(const display::buffer_zero &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] buffer_zero " << _info.destination << '[' << _info.offset << "-"
            << (_info.offset + _info.size) << "]" << std::endl;
#endif

  HUT_PVK_NAMED_ALIASED(vkCmdFillBuffer, ("dst", "dstOffset", "size", "data"),
                        ((void *)_info.destination, _info.offset, _info.size, 0), staging_cb_, _info.destination,
                        _info.offset, _info.size, 0);
}

void display::stage_copy(const image_copy &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] image_copy " << _info.source << '[' << _info.srcOffset.x << "," << _info.srcOffset.y
            << "] to image " << _info.destination << "[" << _info.dstOffset.x << ", " << _info.dstOffset.y << "] size "
            << _info.extent.width << ", " << _info.extent.height << "]" << std::endl;
#endif

  HUT_PVK_NAMED_ALIASED(vkCmdCopyImage, ("src", "dst", "srcOffset", "dstOffset", "extent"),
                        ((void *)_info.source, (void *)_info.destination, offset3_16(_info.srcOffset),
                         offset3_16(_info.dstOffset), extent3_16(_info.extent)),
                        staging_cb_, _info.source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _info.destination,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &_info);
}

void display::stage_copy(const buffer2image_copy &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] buffer2image_copy " << _info.source << '[' << _info.bufferOffset << "-"
            << (_info.bufferOffset + _info.bytesSize) << "] to image " << _info.destination << "["
            << _info.imageOffset.x << ", " << _info.imageOffset.y << ", " << _info.imageExtent.width << ", "
            << _info.imageExtent.height << "]" << std::endl;
#endif

  HUT_PVK_NAMED_ALIASED(vkCmdCopyBufferToImage, ("src", "dst", "srcOffset", "srcSize", "dstOffset", "dstExtent"),
                        ((void *)_info.source, (void *)_info.destination, _info.bufferOffset, _info.bytesSize,
                         offset3_16(_info.imageOffset), extent3_16(_info.imageExtent)),
                        staging_cb_, _info.source, _info.destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &_info);
}

void display::stage_clear(const image_clear &_info, VkImageSubresourceRange _range) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] image_clear " << _info.destination << std::endl;
#endif
  HUT_PVK_NAMED_ALIASED(vkCmdClearColorImage, ("dst", "color"), ((void *)_info.destination, color4_32(_info.color)),
                        staging_cb_, _info.destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &_info.color, 1, &_range);
}

void display::flush_staged() {
  HUT_PROFILE_SCOPE(PDISPLAY, "display::flush_staged");
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

  HUT_PVK(vkEndCommandBuffer, staging_cb_);

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &staging_cb_;

#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] submitting!" << std::endl;
#endif
  HUT_PVK(vkQueueSubmit, queueg_, 1, &submitInfo, VK_NULL_HANDLE);
  HUT_PVK(vkQueueWaitIdle, queueg_);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  HUT_PVK(vkBeginCommandBuffer, staging_cb_, &beginInfo);

  postflush_garbage_.clear();

#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] done, staging pool status:" << std::endl;
  staging_->debug();
#endif
}
