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
#include "hut/utils/vulkan.hpp"

#include "hut/buffer.hpp"
#include "hut/window.hpp"

namespace hut {

#ifdef HUT_ENABLE_VALIDATION
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT _flags,
                                                     VkDebugReportObjectTypeEXT /*_obj_type*/, u64 /*_obj*/,
                                                     size_t /*_location*/, i32 /*_code*/, const char *_prefix,
                                                     const char *_msg, void * /*_data*/) {
  if (strcmp(_prefix, "Loader Message") == 0)
    return VK_FALSE;
  if (strcmp(_prefix, "radv") == 0)
    return VK_FALSE;
  if (strstr(_msg, "SYNC-HAZARD-WRITE_AFTER_WRITE") != nullptr)
    return VK_FALSE;

  char level;
  switch (_flags) {
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT: level = 'I'; break;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT: level = 'W'; break;
    case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT: level = 'P'; break;
    case VK_DEBUG_REPORT_ERROR_BIT_EXT: level = 'E'; break;
    default: level = 'D'; break;
  }

  std::cout << '[' << level << ' ' << _prefix << "] " << _msg << std::endl;
#  if defined(HUT_ENABLE_VALIDATION_DEBUG) && defined(HUT_ENABLE_PROFILING)
  boost::stacktrace::stacktrace st;
  if (st) {
    for (const auto &frame : st.as_vector()) {
      std::cout << "\t[" << _prefix << "] callstack: " << frame << std::endl;
    }
  }
#  endif  //HUT_ENABLE_VALIDATION_DEBUG
  return VK_FALSE;
}
#endif  //HUT_ENABLE_VALIDATION

const char *keysym_name(keysym _keysym) {
  switch (_keysym) {
#define HUT_MAP_KEYSYM(FORMAT_LINUX, FORMAT_X11, FORMAT_IMGUI)                                                         \
  case KSYM_##FORMAT_LINUX:                                                                                            \
    return "KSYM_" #FORMAT_LINUX;
#include "hut/keysyms.inc"
#undef HUT_MAP_KEYSYM
    default: return "KSYM_INVALID";
  }
}

const char *cursor_css_name(cursor_type _c) {
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

const char *format_mime_type(clipboard_format _f) {
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

std::optional<clipboard_format> mime_type_format(const char *_mime_type) {
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

const char *action_name(dragndrop_action _a) {
  switch (_a) {
    case DNDNONE: return "None";
    case DNDMOVE: return "Move";
    case DNDCOPY: return "Copy";
  }
  return nullptr;
}

const char *modifier_name(modifier _m) {
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
  HUT_PROFILE_FUN(PDISPLAY, posted_jobs_.size())
  decltype(posted_jobs_) tmp;
  {
    std::lock_guard lock(posted_mutex_);
    tmp.swap(posted_jobs_);
  }

  for (const auto &job : tmp)
    job(_now);
}

void *display::get_proc_impl(const std::string &_name) {
  static std::unordered_map<std::string, void *> s_cache;

  auto it = s_cache.find(_name);
  if (it == s_cache.end()) {
    auto func = vkGetInstanceProcAddr(instance_, _name.data());
    if (func == nullptr)
      throw std::runtime_error(sstream("can't find address of ") << _name);
    s_cache.emplace(_name, (void *)func);
    return (void *)func;
  } else {
    return it->second;
  }
}

struct score_t {
  constexpr static u32 BAD_ID   = NUMAX<u32>;
  u32                  iqueueg_ = BAD_ID, iqueuec_ = BAD_ID, iqueuet_ = BAD_ID, iqueuep_ = BAD_ID;
  uint                 score_ = 0;
  VkSurfaceFormatKHR   surface_format_{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
};

score_t rate_p_device(VkPhysicalDevice _device, VkSurfaceKHR _dummy) {
  HUT_PROFILE_FUN(PDISPLAY)
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

#ifdef HUT_ENABLE_VALIDATION_DEBUG
  std::cout << "[hut] rating device " << properties.deviceName << " using Vulkan "
            << VK_VERSION_MAJOR(properties.apiVersion) << '.' << VK_VERSION_MINOR(properties.apiVersion) << "."
            << std::endl;
#endif

  u32 extension_count;
  HUT_VVK(HUT_PVK(vkEnumerateDeviceExtensionProperties, _device, nullptr, &extension_count, nullptr));
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  HUT_VVK(
      HUT_PVK(vkEnumerateDeviceExtensionProperties, _device, nullptr, &extension_count, available_extensions.data()));
  bool has_swapchhain_ext = false;
  for (const auto &extension : available_extensions) {
    if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
      has_swapchhain_ext = true;
  }

  if ((_dummy != nullptr) && !has_swapchhain_ext)
    return result;

  if (has_swapchhain_ext) {
    u32 famillies_count;
    HUT_PVK(vkGetPhysicalDeviceQueueFamilyProperties, _device, &famillies_count, nullptr);
    std::vector<VkQueueFamilyProperties> famillies(famillies_count);
    HUT_PVK(vkGetPhysicalDeviceQueueFamilyProperties, _device, &famillies_count, famillies.data());

    for (u32 i = 0; i < famillies_count; i++) {
      const VkQueueFamilyProperties &props = famillies[i];

      if (_dummy != nullptr) {
        VkBool32 present;
        HUT_VVK(HUT_PVK(vkGetPhysicalDeviceSurfaceSupportKHR, _device, i, _dummy, &present));

        if (props.queueCount > 0) {
          // prioritise dedicated queues
          if (result.iqueuep_ == score_t::BAD_ID && (present != 0u))
            result.iqueuep_ = i;
        }

        u32 formats_count;
        u32 modes_count;
        if (HUT_PVK(vkGetPhysicalDeviceSurfacePresentModesKHR, _device, _dummy, &modes_count, nullptr) != VK_SUCCESS)
          modes_count = 0;
        if (HUT_PVK(vkGetPhysicalDeviceSurfaceFormatsKHR, _device, _dummy, &formats_count, nullptr) != VK_SUCCESS)
          formats_count = 0;

        if (modes_count == 0)
          result.score_ = 0;
        if (formats_count == 0)
          result.score_ = 0;
        else {
          std::vector<VkSurfaceFormatKHR> surface_formats(formats_count);
          HUT_PVK(vkGetPhysicalDeviceSurfaceFormatsKHR, _device, _dummy, &formats_count, surface_formats.data());
          result.surface_format_ = surface_formats[0];
          if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
            result.surface_format_ = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
          } else {
            for (const auto &it : surface_formats) {
              if (it.format == VK_FORMAT_B8G8R8A8_UNORM && it.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                result.surface_format_ = it;
                break;
              }
            }
          }
        }
      }

      if (props.queueCount > 0) {
        // prioritise dedicated queues
        if (((props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u)
            && (props.queueFlags & (uint)~VK_QUEUE_GRAPHICS_BIT) == 0)
          result.iqueueg_ = i;
        else if (((props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0u)
                 && (props.queueFlags & (uint)~VK_QUEUE_COMPUTE_BIT) == 0)
          result.iqueuec_ = i;
        else if (((props.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0u)
                 && (props.queueFlags & (uint)~VK_QUEUE_TRANSFER_BIT) == 0)
          result.iqueuet_ = i;

        if (result.iqueueg_ == score_t::BAD_ID && ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u))
          result.iqueueg_ = i;
        if (result.iqueuec_ == score_t::BAD_ID && ((props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0u))
          result.iqueuec_ = i;
        if (result.iqueuet_ == score_t::BAD_ID && ((props.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0u))
          result.iqueuet_ = i;
      }
    }

    if (result.iqueueg_ == score_t::BAD_ID)
      result.score_ = 0;
    if (result.iqueuec_ == score_t::BAD_ID)
      result.score_ = 0;
    if (result.iqueuet_ == score_t::BAD_ID)
      result.score_ = 0;
    if ((_dummy != nullptr) && result.iqueuep_ == score_t::BAD_ID)
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

const std::vector<const char *> G_ENABLED_LAYERS = {
#ifdef HUT_ENABLE_VALIDATION
    "VK_LAYER_KHRONOS_validation",
#endif
};

void display::init_vulkan_instance(const char *_app_name, u32 _app_version, std::vector<const char *> &_extensions) {
  HUT_PROFILE_FUN(PDISPLAY)

#ifdef HUT_ENABLE_VOLK
  HUT_VVK(HUT_PVK(volkInitialize));
#endif

  _extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

  u32 extension_count;
  HUT_PVK(vkEnumerateInstanceExtensionProperties, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  HUT_PVK(vkEnumerateInstanceExtensionProperties, nullptr, &extension_count, available_extensions.data());

  for (const auto &extension : available_extensions) {
#ifdef HUT_ENABLE_VALIDATION
    if (strcmp(extension.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
      _extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
#ifdef HUT_ENABLE_VALIDATION_FEATURES
    if (strcmp(extension.extensionName, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) == 0)
      _extensions.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
#endif
  }

  VkApplicationInfo app_info  = {};
  app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.apiVersion         = VK_API_VERSION_1_2;
  app_info.pEngineName        = "hut";
  app_info.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
  app_info.pApplicationName   = _app_name;
  app_info.applicationVersion = _app_version;

  VkInstanceCreateInfo info    = {};
  info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.enabledExtensionCount   = (u32)_extensions.size();
  info.ppEnabledExtensionNames = _extensions.data();
  info.enabledLayerCount       = (u32)G_ENABLED_LAYERS.size();
  info.ppEnabledLayerNames     = G_ENABLED_LAYERS.data();
  info.pApplicationInfo        = &app_info;

#ifdef HUT_ENABLE_VALIDATION_FEATURES
  std::array<VkValidationFeatureEnableEXT, 5> enabled_validation_features = {
      VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
      VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
      VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
      VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
  };
  VkValidationFeaturesEXT validation_features       = {};
  validation_features.sType                         = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
  validation_features.enabledValidationFeatureCount = enabled_validation_features.size();
  validation_features.pEnabledValidationFeatures    = enabled_validation_features.data();
  info.pNext                                        = &validation_features;
#endif

  HUT_VVK(HUT_PVK(vkCreateInstance, &info, nullptr, &instance_));
#ifdef HUT_ENABLE_VOLK
  HUT_PVK(volkLoadInstanceOnly, instance_);
#endif

#ifdef HUT_ENABLE_VALIDATION
  if (std::find(_extensions.cbegin(), _extensions.cend(), VK_EXT_DEBUG_REPORT_EXTENSION_NAME) != _extensions.cend()) {
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
  HUT_PROFILE_FUN(PDISPLAY)
  u32 device_count;
  HUT_VVK(HUT_PVK(vkEnumeratePhysicalDevices, instance_, &device_count, nullptr));
  std::vector<VkPhysicalDevice> physical_devices(device_count);
  HUT_VVK(HUT_PVK(vkEnumeratePhysicalDevices, instance_, &device_count, physical_devices.data()));

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

  device_features12_       = VkPhysicalDeviceVulkan12Features{};
  device_features12_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  device_features11_       = VkPhysicalDeviceVulkan11Features{};
  device_features11_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
  device_features11_.pNext = &device_features12_;
  device_features_         = VkPhysicalDeviceFeatures2{};
  device_features_.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  device_features_.pNext   = &device_features11_;
  HUT_PVK(vkGetPhysicalDeviceFeatures2, prefered_device, &device_features_);

  device_props12_       = VkPhysicalDeviceVulkan12Properties{};
  device_props12_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
  device_props11_       = VkPhysicalDeviceVulkan11Properties{};
  device_props11_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
  device_props11_.pNext = &device_props12_;
  device_props_         = VkPhysicalDeviceProperties2{};
  device_props_.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  device_props_.pNext   = &device_props11_;
  HUT_PVK(vkGetPhysicalDeviceProperties2, prefered_device, &device_props_);

#ifdef HUT_ENABLE_VALIDATION_DEBUG
  std::cout << "[hut] selected device " << properties().deviceName << " using Vulkan "
            << VK_VERSION_MAJOR(properties().apiVersion) << '.' << VK_VERSION_MINOR(properties().apiVersion)
            << std::endl;
#endif

  pdevice_        = prefered_device;
  iqueueg_        = prefered_rate.iqueueg_;
  iqueuec_        = prefered_rate.iqueuec_;
  iqueuet_        = prefered_rate.iqueuet_;
  iqueuep_        = prefered_rate.iqueuep_;
  surface_format_ = prefered_rate.surface_format_;

  HUT_PVK(vkGetPhysicalDeviceMemoryProperties, pdevice_, &mem_props_);

  std::unordered_set<u32> unique_indexes;
  unique_indexes.emplace(prefered_rate.iqueueg_);
  unique_indexes.emplace(prefered_rate.iqueuec_);
  unique_indexes.emplace(prefered_rate.iqueuet_);
  if (prefered_rate.iqueuep_ != score_t::BAD_ID)
    unique_indexes.emplace(prefered_rate.iqueuep_);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  float                                queue_priority    = 1.0f;
  VkDeviceQueueCreateInfo              queue_create_info = {};
  queue_create_info.sType                                = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueCount                           = 1;
  queue_create_info.pQueuePriorities                     = &queue_priority;
  for (u32 index : unique_indexes) {
    queue_create_info.queueFamilyIndex = index;
    queue_create_infos.emplace_back(queue_create_info);
  }

  u32 extension_count;
  HUT_VVK(HUT_PVK(vkEnumerateDeviceExtensionProperties, prefered_device, nullptr, &extension_count, nullptr));
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  HUT_VVK(HUT_PVK(vkEnumerateDeviceExtensionProperties, prefered_device, nullptr, &extension_count,
                  available_extensions.data()));

  std::vector<const char *> extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  for (const auto &extension : available_extensions) {
#ifdef HUT_ENABLE_PROFILING
    if (strcmp(extension.extensionName, VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME) == 0)
      extensions.emplace_back(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);
#endif
  }

  VkPhysicalDeviceVulkan12Features features12_request          = {};
  features12_request.sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features12_request.shaderSampledImageArrayNonUniformIndexing = features12().shaderSampledImageArrayNonUniformIndexing;
  features12_request.descriptorBindingPartiallyBound           = features12().descriptorBindingPartiallyBound;

  VkPhysicalDeviceVulkan11Features features11_request = {};
  features11_request.sType                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
  features11_request.pNext                            = &features12_request;

  VkPhysicalDeviceFeatures2 features2          = {};
  features2.sType                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2.pNext                              = &features11_request;
  features2.features.samplerAnisotropy         = features().samplerAnisotropy;
  features2.features.multiDrawIndirect         = features().multiDrawIndirect;
  features2.features.drawIndirectFirstInstance = features().drawIndirectFirstInstance;

  VkDeviceCreateInfo device_info      = {};
  device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.pQueueCreateInfos       = queue_create_infos.data();
  device_info.queueCreateInfoCount    = (u32)queue_create_infos.size();
  device_info.pEnabledFeatures        = nullptr;
  device_info.pNext                   = &features2;
  device_info.enabledExtensionCount   = (u32)extensions.size();
  device_info.ppEnabledExtensionNames = extensions.data();
  device_info.enabledLayerCount       = (u32)G_ENABLED_LAYERS.size();
  device_info.ppEnabledLayerNames     = G_ENABLED_LAYERS.data();

  HUT_VVK(HUT_PVK(vkCreateDevice, pdevice_, &device_info, nullptr, &device_));
#ifdef HUT_ENABLE_VOLK
  HUT_PVK(volkLoadDevice, device_);
#endif

  HUT_PVK(vkGetDeviceQueue, device_, prefered_rate.iqueueg_, 0, &queueg_);
  HUT_PVK(vkGetDeviceQueue, device_, prefered_rate.iqueuec_, 0, &queuec_);
  HUT_PVK(vkGetDeviceQueue, device_, prefered_rate.iqueuet_, 0, &queuet_);
  if (prefered_rate.iqueuep_ != score_t::BAD_ID)
    HUT_PVK(vkGetDeviceQueue, device_, prefered_rate.iqueuep_, 0, &queuep_);

  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex        = prefered_rate.iqueueg_;
  pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  HUT_VVK(HUT_PVK(vkCreateCommandPool, device_, &pool_info, nullptr, &commandg_pool_));

  buffer_params staging_params;
  staging_params.permanent_map_ = true;
  staging_params.type_          = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  staging_params.usage_ = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  staging_              = std::make_shared<buffer>(*this, staging_params);

  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool                 = commandg_pool_;
  alloc_info.commandBufferCount          = 1;
  HUT_VVK(HUT_PVK(vkAllocateCommandBuffers, device_, &alloc_info, &staging_cb_));

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  HUT_VVK(HUT_PVK(vkBeginCommandBuffer, staging_cb_, &begin_info));
}

void display::destroy_vulkan() {
  HUT_PROFILE_FUN(PDISPLAY)
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

std::pair<u32, VkMemoryPropertyFlags> display::find_memory_type(u32 _type_filter, VkMemoryPropertyFlags _properties) {
  for (u32 i = 0; i < mem_props_.memoryTypeCount; i++) {
    if (((_type_filter & (1 << i)) != 0u) && (mem_props_.memoryTypes[i].propertyFlags & _properties) == _properties) {
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
                               VkImageLayout _old_layout, VkImageLayout _new_layout) {
  VkImageMemoryBarrier barrier = {};
  barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout            = _old_layout;
  barrier.newLayout            = _new_layout;
  barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                = _image;
  barrier.subresourceRange     = _range;

  VkPipelineStageFlagBits src_stage;
  VkPipelineStageFlagBits dst_stage;

  if (_old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED && _new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    src_stage             = VK_PIPELINE_STAGE_HOST_BIT;
    dst_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (_old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED && _new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    src_stage             = VK_PIPELINE_STAGE_HOST_BIT;
    dst_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (_old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED && _new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    src_stage             = VK_PIPELINE_STAGE_HOST_BIT;
    dst_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else if (_old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             && _new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    src_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dst_stage             = VK_PIPELINE_STAGE_HOST_BIT;
  } else if (_old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
             && _new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    src_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else if (_old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             && _new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    src_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dst_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (_old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
             && _new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    src_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else if (_old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             && _new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    src_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dst_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (_old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
             && _new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    src_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dst_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  HUT_PVK_NAMED_ALIASED(vkCmdPipelineBarrier, ("dest", "from", "to"), ((void *)_image, _old_layout, _new_layout), _cb,
                        src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void display::postflush_collect(buffer_suballoc<u8> &&_callback) {
  postflush_garbage_.emplace_back(std::move(_callback));
}

void display::stage_transition(const image_transition &_info, VkImageSubresourceRange _range) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] transition " << _info.destination_ << " from " << _info.old_layout_ << " to "
            << _info.new_layout_ << std::endl;
#endif

  transition_image(staging_cb_, _info.destination_, _range, _info.old_layout_, _info.new_layout_);
}

void display::stage_copy(const buffer_copy &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] buffer_copy " << _info.source_ << '[' << _info.srcOffset << "-"
            << (_info.srcOffset + _info.size) << "] to " << _info.destination_ << '[' << _info.dstOffset << "-"
            << (_info.dstOffset + _info.size) << ']' << std::endl;
#endif

  HUT_PVK_NAMED_ALIASED(
      vkCmdCopyBuffer, ("src", "srcOffset", "size", "dst", "dstOffset"),
      ((void *)_info.source_, _info.srcOffset, _info.size, (void *)_info.destination_, _info.dstOffset), staging_cb_,
      _info.source_, _info.destination_, 1, &_info);
}

void display::stage_zero(const display::buffer_zero &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] buffer_zero " << _info.destination_ << '[' << _info.offset_ << "-"
            << (_info.offset_ + _info.size_) << "]" << std::endl;
#endif

  HUT_PVK_NAMED_ALIASED(vkCmdFillBuffer, ("dst", "dstOffset", "size", "data"),
                        ((void *)_info.destination_, _info.offset_, _info.size_, 0), staging_cb_, _info.destination_,
                        _info.offset_, _info.size_, 0);
}

void display::stage_copy(const image_copy &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] image_copy " << _info.source_ << '[' << _info.srcOffset.x << "," << _info.srcOffset.y
            << "] to image " << _info.destination_ << "[" << _info.dstOffset.x << ", " << _info.dstOffset.y << "] size "
            << _info.extent.width << ", " << _info.extent.height << "]" << std::endl;
#endif

  HUT_PVK_NAMED_ALIASED(vkCmdCopyImage, ("src", "dst", "srcOffset", "dstOffset", "extent"),
                        ((void *)_info.source_, (void *)_info.destination_, offset3_16(_info.srcOffset),
                         offset3_16(_info.dstOffset), extent3_16(_info.extent)),
                        staging_cb_, _info.source_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _info.destination_,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &_info);
}

void display::stage_copy(const buffer_image_copy &_info) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] buffer2image_copy " << _info.source_ << '[' << _info.bufferOffset << "-"
            << (_info.bufferOffset + _info.bytes_size_) << "] to image " << _info.destination_ << "["
            << _info.imageOffset.x << ", " << _info.imageOffset.y << ", " << _info.imageExtent.width << ", "
            << _info.imageExtent.height << "]" << std::endl;
#endif

  HUT_PVK_NAMED_ALIASED(vkCmdCopyBufferToImage, ("src", "dst", "srcOffset", "srcSize", "dstOffset", "dstExtent"),
                        ((void *)_info.source_, (void *)_info.destination_, _info.bufferOffset, _info.bytes_size_,
                         offset3_16(_info.imageOffset), extent3_16(_info.imageExtent)),
                        staging_cb_, _info.source_, _info.destination_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                        &_info);
}

void display::stage_clear(const image_clear &_info, VkImageSubresourceRange _range) {
#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] image_clear " << _info.destination_ << std::endl;
#endif
  HUT_PVK_NAMED_ALIASED(vkCmdClearColorImage, ("dst", "color"), ((void *)_info.destination_, color4_32(_info.color_)),
                        staging_cb_, _info.destination_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &_info.color_, 1,
                        &_range);
}

void display::flush_staged() {
  HUT_PROFILE_FUN(PDISPLAY)
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

  VkSubmitInfo submit_info       = {};
  submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &staging_cb_;

#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] submitting!" << std::endl;
#endif
  HUT_PVK(vkQueueSubmit, queueg_, 1, &submit_info, VK_NULL_HANDLE);
  HUT_PVK(vkQueueWaitIdle, queueg_);

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  HUT_PVK(vkBeginCommandBuffer, staging_cb_, &begin_info);

  postflush_garbage_.clear();

#ifdef HUT_DEBUG_STAGING
  std::cout << "[staging] done, staging pool status:" << std::endl;
  staging_->debug();
#endif
}

}  // namespace hut
