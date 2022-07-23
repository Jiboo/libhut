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

#if defined(__has_include) && __has_include(<source_location>)
#  include <source_location>
#endif
#include <stdexcept>
#include <system_error>
#include <version>

#ifdef HUT_ENABLE_VOLK
#  include "volk.h"
#else
#  include <vulkan/vulkan.h>
#endif

#include "hut/utils/color.hpp"
#include "hut/utils/length.hpp"

namespace hut {

const inline char *vk_result_name(VkResult _error) {
  switch (_error) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_EVENT_SET: return "VK_EVENT_SET";
    case VK_EVENT_RESET: return "VK_EVENT_RESET";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
    case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
    case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
    default: return "<unknown VkResult>";
  }
};

class vk_result_category : public std::error_category {
 public:
  [[nodiscard]] const char *name() const noexcept override { return "hut::vk_result_category"; }
  [[nodiscard]] std::string message(int _value) const override { return vk_result_name(static_cast<VkResult>(_value)); }
};

static inline vk_result_category g_vk_result_category_instance;

inline std::error_code make_vk_result_error_code(VkResult _code) {
  return {static_cast<int>(_code), g_vk_result_category_instance};
}

// FIXME JBL: Clang 14 fails on libstdc++ impl of std::source_location::current(), for now the CI, on 14.0, uses libc++, but it doesn't provide impl std::source_location
class vulkan_exception : public std::system_error {
 public:
  explicit vulkan_exception(VkResult _result
#ifdef __cpp_lib_source_location
                            ,
                            const std::source_location _location = std::source_location::current()
#endif
                                )
      : std::system_error(make_vk_result_error_code(_result))
#ifdef __cpp_lib_source_location
      , location_(_location)
#endif
  {
  }

  ~vulkan_exception() noexcept override = default;

#ifdef __cpp_lib_source_location
  [[nodiscard]] const std::source_location &location() const noexcept { return location_; }

 protected:
  std::source_location location_;
#endif
};

#define HUT_VVK(EXPR)                                                                                                  \
  if (VkResult res = EXPR; res != VK_SUCCESS) {                                                                        \
    throw vulkan_exception(res);                                                                                       \
  }

inline i16vec2_px offset2_16(VkOffset2D _in) {
  return i16vec2_px{_in.x, _in.y};
}
inline i16vec3_px offset3_16(VkOffset3D _in) {
  return i16vec3_px{_in.x, _in.y, _in.z};
}
inline u16vec2_px extent2_16(VkExtent2D _in) {
  return u16vec2_px{_in.width, _in.height};
}
inline u16vec3_px extent3_16(VkExtent3D _in) {
  return u16vec3_px{_in.width, _in.height, _in.depth};
}
inline f32vec4_rgba color4_32(VkClearColorValue _in) {
  return f32vec4_rgba{_in.float32[0], _in.float32[1], _in.float32[2], _in.float32[3]};
}

}  // namespace hut
