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

#include <algorithm>
#include <iostream>
#include <thread>

#include "hut/display.hpp"
#include "hut/window.hpp"
#include "hut/image.hpp"

using namespace hut;

cursor_type hut::edge_cursor(edge _edge) {
  switch (_edge.active_) {
    case edge(TOP).active_: return CRESIZE_N;
    case edge(RIGHT).active_: return CRESIZE_E;
    case edge(LEFT).active_: return CRESIZE_W;
    case edge(BOTTOM).active_: return CRESIZE_S;

    case edge({TOP, RIGHT}).active_: return CRESIZE_NE;
    case edge({TOP, LEFT}).active_: return CRESIZE_NW;
    case edge({BOTTOM, RIGHT}).active_: return CRESIZE_SE;
    case edge({BOTTOM, LEFT}).active_: return CRESIZE_SW;
  }
  return CDEFAULT;
}

void window::invalidate(bool _redraw) {
  invalidate(uvec4{uvec2{0, 0}, size_}, _redraw);
}

void window::destroy_vulkan() {
  if (surface_ == VK_NULL_HANDLE)
    return;

  HUT_PVK(vkDeviceWaitIdle, display_.device_);

  if (sem_available_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySemaphore, display_.device_, sem_available_, nullptr);

  if (sem_rendered_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySemaphore, display_.device_, sem_rendered_, nullptr);

  if (renderpass_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroyRenderPass, display_.device_, renderpass_, nullptr);

  for (auto &fbo : swapchain_fbos_) {
    if (fbo != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyFramebuffer, display_.device_, fbo, nullptr);
  }

  for (auto &view : swapchain_imageviews_) {
    if (view != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyImageView, display_.device_, view, nullptr);
  }

  if (swapchain_ != VK_NULL_HANDLE) {
    HUT_PVK(vkDestroySwapchainKHR, display_.device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  if (surface_ != VK_NULL_HANDLE) {
    HUT_PVK(vkDestroySurfaceKHR, display_.instance_, surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }
}

VkPresentModeKHR select_best_mode(const span<VkPresentModeKHR> &_modes)
{
  VkPresentModeKHR preferred_modes[] = {
      VK_PRESENT_MODE_MAILBOX_KHR,
      VK_PRESENT_MODE_IMMEDIATE_KHR,
      VK_PRESENT_MODE_FIFO_RELAXED_KHR,
      VK_PRESENT_MODE_FIFO_KHR,
  };

  for (const auto &preferred_mode : preferred_modes) {
    for (const auto &mode : _modes) {
      if (mode == preferred_mode) {
        return preferred_mode;
      }
    }
  }

  assert(false); // VK_PRESENT_MODE_FIFO_KHR should at least have been selected
  return VK_PRESENT_MODE_MAX_ENUM_KHR;
}

void window::init_vulkan_surface() {
  HUT_PROFILE_SCOPE(PWINDOW, "window::init_vulkan_surface");
  if (surface_ == VK_NULL_HANDLE)
    return;

  const VkPhysicalDevice &pdevice = display_.pdevice_;

  // FIXME JBL: No need to request thus formats every resize, cache them or extract from there
  VkBool32 present;
  HUT_PVK(vkGetPhysicalDeviceSurfaceSupportKHR, pdevice, display_.iqueuep_, surface_, &present);
  if (!present)
    throw std::runtime_error("can't create a swapchain for the provided surface");

  VkSurfaceCapabilitiesKHR capabilities = {};
  HUT_PVK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, pdevice, surface_, &capabilities);

  uint32_t formats_count;
  HUT_PVK(vkGetPhysicalDeviceSurfaceFormatsKHR, pdevice, surface_, &formats_count, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(formats_count);
  HUT_PVK(vkGetPhysicalDeviceSurfaceFormatsKHR, pdevice, surface_, &formats_count, formats.data());
  surface_format_ = formats[0];
  if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
    surface_format_ = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  } else {
    for (const auto &it : formats) {
      if (it.format == VK_FORMAT_B8G8R8A8_UNORM && it.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        surface_format_ = it;
      }
    }
  }
#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
  VkFormat depth_format = VK_FORMAT_UNDEFINED;
  VkFormat depth_candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
  for (VkFormat format : depth_candidates) {
    VkFormatProperties props;
    HUT_PVK(vkGetPhysicalDeviceFormatProperties, pdevice, format, &props);

    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      depth_format = format;
      break;
    }
  }
  if (depth_format == VK_FORMAT_UNDEFINED)
    throw std::runtime_error("failed to find compatible depth buffer!");

  depth_ = std::make_shared<image>(display_, size_, depth_format, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
#endif

  uint32_t modes_count;
  HUT_PVK(vkGetPhysicalDeviceSurfacePresentModesKHR, pdevice, surface_, &modes_count, nullptr);
  std::vector<VkPresentModeKHR> modes(modes_count);
  HUT_PVK(vkGetPhysicalDeviceSurfacePresentModesKHR, pdevice, surface_, &modes_count, modes.data());
  present_mode_ = params_.flags_ & window_params::VSYNC ? VK_PRESENT_MODE_FIFO_KHR : select_best_mode(modes);

  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    swapchain_extents_ = capabilities.currentExtent;
  } else {
    VkExtent2D tmp = {size_.x, size_.y};

    tmp.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, tmp.width));
    tmp.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, tmp.height));

    swapchain_extents_ = tmp;
  }

  uint32_t images_count = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && images_count > capabilities.maxImageCount) {
    images_count = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapchain_infos = {};
  swapchain_infos.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_infos.surface = surface_;
  swapchain_infos.minImageCount = images_count;
  swapchain_infos.imageFormat = surface_format_.format;
  swapchain_infos.imageColorSpace = surface_format_.colorSpace;
  swapchain_infos.imageExtent = swapchain_extents_;
  swapchain_infos.imageArrayLayers = 1;
  swapchain_infos.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t iqueues[] = {display_.iqueueg_, display_.iqueuep_};
  if (iqueues[0] != iqueues[1]) {
    swapchain_infos.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_infos.queueFamilyIndexCount = 2;
    swapchain_infos.pQueueFamilyIndices = iqueues;
  } else {
    swapchain_infos.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_infos.queueFamilyIndexCount = 0;
    swapchain_infos.pQueueFamilyIndices = nullptr;
  }
  swapchain_infos.preTransform = capabilities.currentTransform;
  swapchain_infos.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_infos.presentMode = present_mode_;
  swapchain_infos.clipped = VK_TRUE;
  VkSwapchainKHR old_swapchain = swapchain_;
  swapchain_infos.oldSwapchain = old_swapchain;

  VkSwapchainKHR new_swapchain;
  if (HUT_PVK(vkCreateSwapchainKHR, display_.device_, &swapchain_infos, nullptr, &new_swapchain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  HUT_PVK(vkDeviceWaitIdle, display_.device_);

  swapchain_ = new_swapchain;
  if (old_swapchain != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySwapchainKHR, display_.device_, old_swapchain, nullptr);

  HUT_PVK(vkGetSwapchainImagesKHR, display_.device_, swapchain_, &images_count, nullptr);
  swapchain_images_.resize(images_count);
  HUT_PVK(vkGetSwapchainImagesKHR, display_.device_, swapchain_, &images_count, swapchain_images_.data());

  for (auto &imageview : swapchain_imageviews_) {
    if (imageview != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyImageView, display_.device_, imageview, nullptr);
  }
  swapchain_imageviews_.resize(images_count, VK_NULL_HANDLE);
  for (uint32_t i = 0; i < images_count; i++) {
    VkImageViewCreateInfo imagev_infos = {};
    imagev_infos.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imagev_infos.image = swapchain_images_[i];
    imagev_infos.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imagev_infos.format = surface_format_.format;
    imagev_infos.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imagev_infos.subresourceRange.baseMipLevel = 0;
    imagev_infos.subresourceRange.levelCount = 1;
    imagev_infos.subresourceRange.baseArrayLayer = 0;
    imagev_infos.subresourceRange.layerCount = 1;
    if (HUT_PVK(vkCreateImageView, display_.device_, &imagev_infos, nullptr, &swapchain_imageviews_[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create image views!");
  }

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = surface_format_.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = depth_->format_;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
#endif

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
#endif

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkAttachmentDescription attachments[] = {
    colorAttachment,
#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
    depthAttachment,
#endif
  };

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = sizeof(attachments) / sizeof(VkAttachmentDescription);
  renderPassInfo.pAttachments = attachments;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (renderpass_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroyRenderPass, display_.device_, renderpass_, nullptr);
  if (HUT_PVK(vkCreateRenderPass, display_.device_, &renderPassInfo, nullptr, &renderpass_) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass!");

  for (auto &fbo : swapchain_fbos_) {
    if (fbo != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyFramebuffer, display_.device_, fbo, nullptr);
  }
  swapchain_fbos_.resize(swapchain_images_.size());
  for (size_t i = 0; i < swapchain_fbos_.size(); i++) {
    VkImageView attachments[] = {
        swapchain_imageviews_[i],
#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
        depth_->view_,
#endif
    };

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderpass_;
    framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(VkImageView);
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapchain_extents_.width;
    framebufferInfo.height = swapchain_extents_.height;
    framebufferInfo.layers = 1;

    if (HUT_PVK(vkCreateFramebuffer, display_.device_, &framebufferInfo, nullptr, &swapchain_fbos_[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create framebuffer!");
  }

  if (!primary_cbs_.empty())
    HUT_PVK(vkFreeCommandBuffers, display_.device_, display_.commandg_pool_, primary_cbs_.size(), primary_cbs_.data());

  primary_cbs_.resize(images_count);
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = images_count;
  allocInfo.commandPool = display_.commandg_pool_;

  if (HUT_PVK(vkAllocateCommandBuffers, display_.device_, &allocInfo, primary_cbs_.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");

  if (sem_available_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySemaphore, display_.device_, sem_available_, nullptr);
  if (sem_rendered_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroySemaphore, display_.device_, sem_rendered_, nullptr);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  if (HUT_PVK(vkCreateSemaphore, display_.device_, &semaphoreInfo, nullptr, &sem_available_) != VK_SUCCESS ||
      HUT_PVK(vkCreateSemaphore, display_.device_, &semaphoreInfo, nullptr, &sem_rendered_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create semaphores!");
  }

  invalidate(true);
  dirty_.resize(images_count, true);
}

void window::redraw(display::time_point _tp) {
  HUT_PROFILE_SCOPE(PWINDOW, "window::redraw");
  if (swapchain_ == VK_NULL_HANDLE)
    return;

  uint32_t imageIndex;
  VkResult result = HUT_PVK(vkAcquireNextImageKHR, display_.device_, swapchain_, std::numeric_limits<uint64_t>::max(),
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
    rebuild_cb(swapchain_fbos_[imageIndex], primary_cbs_[imageIndex]);
  }

  HUT_PROFILE_EVENT(this, on_frame, _tp - last_frame_);
  display_.flush_staged();
  cbs_.emplace_back(primary_cbs_[imageIndex]);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {sem_available_};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = (uint32_t)cbs_.size();
  submitInfo.pCommandBuffers = cbs_.data();

  VkSemaphore signalSemaphores[] = {sem_rendered_};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (HUT_PVK(vkQueueSubmit, display_.queueg_, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer!");

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapchain_};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr;  // Optional

  result = HUT_PVK(vkQueuePresentKHR, display_.queuep_, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    init_vulkan_surface();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  cbs_.clear();

  auto done = display::clock::now();
  auto diff_frame = done - last_frame_;
  if (diff_frame > 20ms) {
    profiling::threads_data::request_dump();
  }
  last_frame_ = done;
}

void window::rebuild_cb(VkFramebuffer _fbo, VkCommandBuffer _cb) {
  HUT_PROFILE_SCOPE(PWINDOW, "window::rebuild_cb");
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;  // Optional

  HUT_PVK(vkBeginCommandBuffer, _cb, &beginInfo);

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderpass_;
  renderPassInfo.framebuffer = _fbo;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapchain_extents_;

  VkClearValue clearColors[] = {
      {.color = {{clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a}}},
#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
      {.depthStencil = {1.f, 0}},
#endif
  };
  renderPassInfo.clearValueCount = sizeof(clearColors) / sizeof(VkClearValue);
  renderPassInfo.pClearValues = clearColors;

  HUT_PVK(vkCmdBeginRenderPass, _cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);  // FIXME
  // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
  // to enable secondary command buffers

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = {size_.x, size_.y};
  HUT_PVK(vkCmdSetScissor, _cb, 0, 1, &scissor);

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)size_.x;
  viewport.height = (float)size_.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  HUT_PVK(vkCmdSetViewport, _cb, 0, 1, &viewport);

  HUT_PROFILE_EVENT_NAMED_ALIASED(this, on_draw, (), (), _cb);

  HUT_PVK(vkCmdEndRenderPass, _cb);
  if (HUT_PVK(vkEndCommandBuffer, _cb) != VK_SUCCESS)
    throw std::runtime_error("failed to record command buffer!");
}

std::string hut::name_key(keysym c) {
  if (c > KENUM_START && c < KENUM_END) {
    switch (c) {
#define HUT_NAME_KEY(CASE) case CASE: return #CASE;
      HUT_NAME_KEY(KTAB)
      HUT_NAME_KEY(KALT_LEFT)
      HUT_NAME_KEY(KALT_RIGHT)
      HUT_NAME_KEY(KCTRL_LEFT)
      HUT_NAME_KEY(KCTRL_RIGHT)
      HUT_NAME_KEY(KSHIFT_LEFT)
      HUT_NAME_KEY(KSHIFT_RIGHT)
      HUT_NAME_KEY(KPAGE_DOWN)
      HUT_NAME_KEY(KPAGE_UP)
      HUT_NAME_KEY(KUP)
      HUT_NAME_KEY(KRIGHT)
      HUT_NAME_KEY(KDOWN)
      HUT_NAME_KEY(KLEFT)
      HUT_NAME_KEY(KHOME)
      HUT_NAME_KEY(KEND)
      HUT_NAME_KEY(KRETURN)
      HUT_NAME_KEY(KBACKSPACE)
      HUT_NAME_KEY(KDELETE)
      HUT_NAME_KEY(KINSER)
      HUT_NAME_KEY(KESCAPE)
      HUT_NAME_KEY(KF1)
      HUT_NAME_KEY(KF2)
      HUT_NAME_KEY(KF3)
      HUT_NAME_KEY(KF4)
      HUT_NAME_KEY(KF5)
      HUT_NAME_KEY(KF6)
      HUT_NAME_KEY(KF7)
      HUT_NAME_KEY(KF8)
      HUT_NAME_KEY(KF9)
      HUT_NAME_KEY(KF10)
      HUT_NAME_KEY(KF11)
      HUT_NAME_KEY(KF12)
#undef HUT_NAME_KEY
      case KENUM_START: break;
      case KENUM_END: break;
    }
  }
  return to_utf8(c);
}
