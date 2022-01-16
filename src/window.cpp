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

#include "hut/window.hpp"

#include <algorithm>
#include <iostream>
#include <thread>

#include "hut/utils/profiling.hpp"

#include "hut/display.hpp"
#include "hut/image.hpp"

using namespace hut;

const char *hut::touch_event_name(touch_event_type _type) {
  switch (_type) {
    case TDOWN: return "TDOWN";
    case TUP: return "TUP";
    case TMOVE: return "TMOVE";
    default: return "invalid_touch_event_type";
  }
}

const char *hut::mouse_event_name(mouse_event_type _type) {
  switch (_type) {
    case MDOWN: return "MDOWN";
    case MUP: return "MUP";
    case MMOVE: return "MMOVE";
    case MWHEEL_UP: return "MWHEEL_UP";
    case MWHEEL_DOWN: return "MWHEEL_DOWN";
    default: return "invalid_mouse_event_type";
  }
}

cursor_type hut::edge_cursor(edge _edge) {
  switch (_edge.raw()) {
    case edge{TOP}.raw(): return CRESIZE_N;
    case edge{RIGHT}.raw(): return CRESIZE_E;
    case edge{LEFT}.raw(): return CRESIZE_W;
    case edge{BOTTOM}.raw(): return CRESIZE_S;

    case edge{TOP, RIGHT}.raw(): return CRESIZE_NE;
    case edge{TOP, LEFT}.raw(): return CRESIZE_NW;
    case edge{BOTTOM, RIGHT}.raw(): return CRESIZE_SE;
    case edge{BOTTOM, LEFT}.raw(): return CRESIZE_SW;
  }
  return CDEFAULT;
}

void window::invalidate(bool _redraw) {
  invalidate(uvec4{uvec2{0, 0}, size_}, _redraw);
}

void window::destroy_vulkan() {
  if (surface_ == VK_NULL_HANDLE)
    return;

  HUT_PVK(vkDeviceWaitIdle, display_.device());

  if (sem_available_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySemaphore, display_.device(), sem_available_, nullptr);

  if (sem_rendered_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySemaphore, display_.device(), sem_rendered_, nullptr);

  for (auto &view : swapchain_imageviews_) {
    if (view != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyImageView, display_.device(), view, nullptr);
  }

  if (swapchain_ != VK_NULL_HANDLE) {
    HUT_PVK(vkDestroySwapchainKHR, display_.device(), swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  if (surface_ != VK_NULL_HANDLE) {
    HUT_PVK(vkDestroySurfaceKHR, display_.instance(), surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }
}

VkPresentModeKHR select_best_mode(const std::span<VkPresentModeKHR> &_modes, bool _vsync_only) {
  constexpr VkPresentModeKHR preferred_tearing_modes[] = {
      VK_PRESENT_MODE_IMMEDIATE_KHR,
      VK_PRESENT_MODE_MAILBOX_KHR,
  };
  constexpr VkPresentModeKHR preferred_vsync_modes[] = {
      VK_PRESENT_MODE_FIFO_RELAXED_KHR,
      VK_PRESENT_MODE_FIFO_KHR,
  };

  if (!_vsync_only) {
    for (const auto &preferred_mode : preferred_tearing_modes) {
      for (const auto &mode : _modes) {
        if (mode == preferred_mode)
          return preferred_mode;
      }
    }
  }
  for (const auto &preferred_mode : preferred_vsync_modes) {
    for (const auto &mode : _modes) {
      if (mode == preferred_mode)
        return preferred_mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

void window::init_vulkan_surface() {
  HUT_PROFILE_SCOPE(PWINDOW, "window::init_vulkan_surface");
  if (surface_ == VK_NULL_HANDLE)
    return;

  const VkPhysicalDevice &pdevice = display_.pdevice();

#ifdef HUT_ENABLE_VALIDATION_DEBUG
  // NOTE JBL: This should be validated at device selection (we eliminate devices that can't present, if dummy surface is provided)
  VkBool32 present;
  HUT_PVK(vkGetPhysicalDeviceSurfaceSupportKHR, pdevice, display_.iqueuep_, surface_, &present);
  if (!present)
    throw std::runtime_error("can't create a swapchain for the provided surface");
#endif

  VkSurfaceCapabilitiesKHR capabilities = {};
  HUT_PVK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, pdevice, surface_, &capabilities);
  if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
    swapchain_extents_ = capabilities.currentExtent;
  } else {
    swapchain_extents_ = {size_.x, size_.y};
  }
  swapchain_extents_.width  = std::max(capabilities.minImageExtent.width,
                                       std::min(capabilities.maxImageExtent.width, swapchain_extents_.width));
  swapchain_extents_.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, swapchain_extents_.height));

  u32 images_count = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && images_count > capabilities.maxImageCount) {
    images_count = capabilities.maxImageCount;
  }

  if (present_mode_ == VK_PRESENT_MODE_MAX_ENUM_KHR) {
    u32 modes_count;
    HUT_PVK(vkGetPhysicalDeviceSurfacePresentModesKHR, pdevice, surface_, &modes_count, nullptr);
    std::vector<VkPresentModeKHR> modes(modes_count);
    HUT_PVK(vkGetPhysicalDeviceSurfacePresentModesKHR, pdevice, surface_, &modes_count, modes.data());
    present_mode_ = select_best_mode(modes, params_.flags_.query(window_params::FVSYNC));
  }

  VkSwapchainCreateInfoKHR swapchain_infos = {};
  swapchain_infos.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_infos.surface                  = surface_;
  swapchain_infos.minImageCount            = images_count;
  swapchain_infos.imageFormat              = display_.surface_format_.format;
  swapchain_infos.imageColorSpace          = display_.surface_format_.colorSpace;
  swapchain_infos.imageExtent              = swapchain_extents_;
  swapchain_infos.imageArrayLayers         = 1;
  swapchain_infos.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  u32 iqueues[] = {display_.iqueueg_, display_.iqueuep_};
  if (iqueues[0] != iqueues[1]) {
    swapchain_infos.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    swapchain_infos.queueFamilyIndexCount = 2;
    swapchain_infos.pQueueFamilyIndices   = iqueues;
  } else {
    swapchain_infos.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_infos.queueFamilyIndexCount = 0;
    swapchain_infos.pQueueFamilyIndices   = nullptr;
  }
  swapchain_infos.preTransform   = capabilities.currentTransform;
  swapchain_infos.compositeAlpha = params_.flags_ & window_params::FTRANSPARENT
                                     ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
                                     : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_infos.presentMode    = present_mode_;
  swapchain_infos.clipped        = VK_TRUE;
  VkSwapchainKHR old_swapchain   = swapchain_;
  swapchain_infos.oldSwapchain   = old_swapchain;

  VkSwapchainKHR new_swapchain;
  if (HUT_PVK(vkCreateSwapchainKHR, display_.device(), &swapchain_infos, nullptr, &new_swapchain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  HUT_PVK(vkDeviceWaitIdle, display_.device());

  swapchain_ = new_swapchain;
  if (old_swapchain != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySwapchainKHR, display_.device(), old_swapchain, nullptr);

  HUT_PVK(vkGetSwapchainImagesKHR, display_.device(), swapchain_, &images_count, nullptr);
  swapchain_images_.resize(images_count);
  HUT_PVK(vkGetSwapchainImagesKHR, display_.device(), swapchain_, &images_count, swapchain_images_.data());

  for (auto &imageview : swapchain_imageviews_) {
    if (imageview != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyImageView, display_.device(), imageview, nullptr);
  }
  swapchain_imageviews_.resize(images_count, VK_NULL_HANDLE);
  for (u32 i = 0; i < images_count; i++) {
    VkImageViewCreateInfo imagev_infos           = {};
    imagev_infos.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imagev_infos.image                           = swapchain_images_[i];
    imagev_infos.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imagev_infos.format                          = display_.surface_format_.format;
    imagev_infos.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imagev_infos.subresourceRange.baseMipLevel   = 0;
    imagev_infos.subresourceRange.levelCount     = 1;
    imagev_infos.subresourceRange.baseArrayLayer = 0;
    imagev_infos.subresourceRange.layerCount     = 1;
    if (HUT_PVK(vkCreateImageView, display_.device_, &imagev_infos, nullptr, &swapchain_imageviews_[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create image views!");
  }

  render_target_params pass_params;
  pass_params.clear_color_         = render_target_params_.clear_color_;
  pass_params.clear_depth_stencil_ = render_target_params_.clear_depth_stencil_;
  pass_params.box_                 = {0, 0, swapchain_extents_.width, swapchain_extents_.height};
  pass_params.format_              = display_.surface_format_.format;
  if (params_.flags_ & window_params::FMULTISAMPLING)
    pass_params.flags_ |= render_target_params::FMULTISAMPLING;
  if (params_.flags_ & window_params::FDEPTH)
    pass_params.flags_ |= render_target_params::FDEPTH;
  reinit_pass(pass_params, swapchain_imageviews_);

  if (!primary_cbs_.empty())
    HUT_PVK(vkFreeCommandBuffers, display_.device_, display_.commandg_pool_, primary_cbs_.size(), primary_cbs_.data());

  primary_cbs_.resize(images_count);
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount          = images_count;
  allocInfo.commandPool                 = display_.commandg_pool_;

  if (HUT_PVK(vkAllocateCommandBuffers, display_.device_, &allocInfo, primary_cbs_.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");

  if (sem_available_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySemaphore, display_.device_, sem_available_, nullptr);
  if (sem_rendered_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySemaphore, display_.device_, sem_rendered_, nullptr);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  if (HUT_PVK(vkCreateSemaphore, display_.device_, &semaphoreInfo, nullptr, &sem_available_) != VK_SUCCESS
      || HUT_PVK(vkCreateSemaphore, display_.device_, &semaphoreInfo, nullptr, &sem_rendered_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create semaphores!");
  }

  invalidate(true);
  dirty_.resize(images_count, true);
}

void window::redraw(display::time_point _tp) {
  HUT_PROFILE_SCOPE(PWINDOW, "window::redraw");
  if (swapchain_ == VK_NULL_HANDLE)
    return;

  u32      imageIndex;
  VkResult result = HUT_PVK(vkAcquireNextImageKHR, display_.device_, swapchain_, std::numeric_limits<u64>::max(),
                            sem_available_, VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    init_vulkan_surface();
    invalidate(false);
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  if (dirty_[imageIndex]) {
    HUT_PVK(vkDeviceWaitIdle, display_.device_);
    dirty_[imageIndex] = false;
    begin_rebuild_cb(fbos_[imageIndex], primary_cbs_[imageIndex]);
    HUT_PROFILE_EVENT_NAMED_ALIASED(this, on_draw, (), (), primary_cbs_[imageIndex]);
    end_rebuild_cb(primary_cbs_[imageIndex]);
  }

  HUT_PROFILE_EVENT(this, on_frame, _tp - last_frame_);
  display_.flush_staged();
  cbs_.emplace_back(primary_cbs_[imageIndex]);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore          waitSemaphores[] = {sem_available_};
  VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount         = 1;
  submitInfo.pWaitSemaphores            = waitSemaphores;
  submitInfo.pWaitDstStageMask          = waitStages;

  submitInfo.commandBufferCount = (u32)cbs_.size();
  submitInfo.pCommandBuffers    = cbs_.data();

  VkSemaphore signalSemaphores[]  = {sem_rendered_};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  if (HUT_PVK(vkQueueSubmit, display_.queueg_, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer!");

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapchain_};
  presentInfo.swapchainCount  = 1;
  presentInfo.pSwapchains     = swapChains;
  presentInfo.pImageIndices   = &imageIndex;
  presentInfo.pResults        = nullptr;  // Optional

  result = HUT_PVK(vkQueuePresentKHR, display_.queuep_, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    init_vulkan_surface();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  cbs_.clear();

  constexpr auto min_frame_time = 1000ms / 144.f;
  constexpr auto max_frame_time = 1000ms / 10.f;
  auto           done           = display::clock::now();
  auto           diff_frame     = done - last_frame_;
  if (diff_frame > max_frame_time) {
#ifdef HUT_ENABLE_VALIDATION_DEBUG
    std::cout << "Frame overbudget " << diff_frame << " > " << max_frame_time << std::endl;
#endif
    profiling::threads_data::request_dump();
  }
#ifdef HUT_PROFILE_BOOT
  static bool profile_boot_dumped = false;
  if (!profile_boot_dumped) {
    profiling::threads_data::request_dump();
    profile_boot_dumped = true;
  }
#endif
  last_frame_ = done;
}
