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
#include <string>

#include <vulkan/vulkan.h>

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#endif

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "hut/utils.hpp"

namespace hut {

enum touch_event_type { TDOWN, TUP, TMOVE };

enum mouse_event_type { MDOWN, MUP, MMOVE, MENTER, MLEAVE, MWHEEL_UP, MWHEEL_DOWN };

enum keysym : char32_t {
  KENUM_START = 0xffffff00,
  KTAB,
  KALT_LEFT,
  KALT_RIGHT,
  KCTRL_LEFT,
  KCTRL_RIGHT,
  KSHIFT_LEFT,
  KSHIFT_RIGHT,
  KPAGE_UP,
  KPAGE_DOWN,
  KUP,
  KRIGHT,
  KDOWN,
  KLEFT,
  KHOME,
  KEND,
  KRETURN,
  KBACKSPACE,
  KDELETE,
  KINSER,
  KESCAPE,
  KF1,
  KF2,
  KF3,
  KF4,
  KF5,
  KF6,
  KF7,
  KF8,
  KF9,
  KF10,
  KF11,
  KF12,
  KENUM_END
};

class display;

class window {
  friend class display;
  friend class noinput;
  friend class rgb;
  friend class rgba;
  friend class tex;
  friend class rgb_tex;
  friend class rgba_tex;

 public:
  event<> on_pause, on_resume, on_focus, on_blur, on_close;
  event<glm::uvec4> on_expose;
  event<glm::uvec2> on_resize;
  event<VkCommandBuffer, glm::uvec2 /*canvas_size*/> on_draw;
  event<glm::uvec2, display::duration /*delta*/> on_frame;
  event<std::string /*path*/, glm::uvec2 /*pos*/> on_drop;

  event<uint8_t /*finger*/, touch_event_type, glm::uvec2 /*pos*/> on_touch;
  event<uint8_t /*button*/, mouse_event_type, glm::uvec2 /*pos*/> on_mouse;

  event<char32_t /*utf32_char*/, bool /*down*/> on_keysym;
  static bool is_keypad_key(char32_t c);
  static bool is_cursor_key(char32_t c);
  static bool is_function_key(char32_t c);
  static bool is_modifier_key(char32_t c);
  static keysym map_key(char32_t c);
  static std::string name_key(char32_t c);

  window(display &_display);
  ~window();

  void close();

  bool visible() {
    return visible_;
  }
  void visible(bool);
  void title(const std::string &);
  void invalidate(const glm::uvec4 &, bool _redraw);
  void invalidate(bool _redraw);

  glm::uvec2 size() {
    return size_;
  }

  void clear_color(const glm::vec4 &_color) {
    clear_color_ = _color;
    invalidate(true);
  }

 protected:
  display &display_;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  VkPresentModeKHR present_mode_;
  VkSurfaceFormatKHR surface_format_;
  VkRenderPass renderpass_ = VK_NULL_HANDLE;

  VkExtent2D swapchain_extents_;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_imageviews_;
  std::vector<VkFramebuffer> swapchain_fbos_;
  std::vector<VkCommandBuffer> primary_cbs_;
  std::vector<VkCommandBuffer> cbs_;
  std::vector<bool> dirty_;

  VkSemaphore sem_available_ = VK_NULL_HANDLE;
  VkSemaphore sem_rendered_ = VK_NULL_HANDLE;

  bool visible_ = false;
  uint16_t fps_limit_ = 0;
  glm::uvec2 size_;
  glm::vec4 clear_color_ = {0.0f, 0.0f, 0.0f, 1.0f};
  display::time_point last_frame_ = display::clock::now();

  void init_vulkan_surface();
  void dispatch_resize(const glm::uvec2 &);
  void rebuild_cb(VkFramebuffer _fbo, VkCommandBuffer _cb);
  void destroy_vulkan();
  void redraw(display::time_point);

#if defined(VK_USE_PLATFORM_XCB_KHR)
 protected:
  xcb_window_t window_, parent_;
#endif
};

}  // namespace hut
