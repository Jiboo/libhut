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

#include <iomanip>
#include <iostream>
#include <thread>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

window::~window() {
  close();
}

void window::invalidate(bool _redraw) {
  invalidate(uvec4{uvec2{0, 0}, size_}, _redraw);
}

void window::destroy_vulkan() {
  display_.check_thread();

  if (surface_ == VK_NULL_HANDLE)
    return;

  vkDeviceWaitIdle(display_.device_);

  if (sem_available_ != VK_NULL_HANDLE)
    vkDestroySemaphore(display_.device_, sem_available_, nullptr);

  if (sem_rendered_ != VK_NULL_HANDLE)
    vkDestroySemaphore(display_.device_, sem_rendered_, nullptr);

  if (renderpass_ != VK_NULL_HANDLE)
    vkDestroyRenderPass(display_.device_, renderpass_, nullptr);

  for (auto &fbo : swapchain_fbos_) {
    if (fbo != VK_NULL_HANDLE)
      vkDestroyFramebuffer(display_.device_, fbo, nullptr);
  }

  for (auto &view : swapchain_imageviews_) {
    if (view != VK_NULL_HANDLE)
      vkDestroyImageView(display_.device_, view, nullptr);
  }

  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(display_.device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  if (surface_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(display_.instance_, surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }
}

void window::init_vulkan_surface() {
  display_.check_thread();

  if (surface_ == VK_NULL_HANDLE)
    return;

  const VkPhysicalDevice &pdevice = display_.pdevice_;

  VkBool32 present;
  vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, display_.iqueuep_, surface_, &present);
  if (!present)
    throw std::runtime_error("can't create a swapchain for the provided surface");

  VkSurfaceCapabilitiesKHR capabilities = {};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice, surface_, &capabilities);

  uint32_t formats_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, surface_, &formats_count, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(formats_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, surface_, &formats_count, formats.data());
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

  uint32_t modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice, surface_, &modes_count, nullptr);
  std::vector<VkPresentModeKHR> modes(modes_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice, surface_, &modes_count, modes.data());
  present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
  /*for (const auto &it : modes) {
    if (it == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode_ = it;
    }
  }*/

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
  if (vkCreateSwapchainKHR(display_.device_, &swapchain_infos, nullptr, &new_swapchain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  vkDeviceWaitIdle(display_.device_);

  swapchain_ = new_swapchain;
  if (old_swapchain != VK_NULL_HANDLE)
    vkDestroySwapchainKHR(display_.device_, old_swapchain, nullptr);

  vkGetSwapchainImagesKHR(display_.device_, swapchain_, &images_count, nullptr);
  swapchain_images_.resize(images_count);
  vkGetSwapchainImagesKHR(display_.device_, swapchain_, &images_count, swapchain_images_.data());

  for (auto &imageview : swapchain_imageviews_) {
    if (imageview != VK_NULL_HANDLE)
      vkDestroyImageView(display_.device_, imageview, nullptr);
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
    if (vkCreateImageView(display_.device_, &imagev_infos, nullptr, &swapchain_imageviews_[i]) != VK_SUCCESS)
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

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (renderpass_ != VK_NULL_HANDLE)
    vkDestroyRenderPass(display_.device_, renderpass_, nullptr);
  if (vkCreateRenderPass(display_.device_, &renderPassInfo, nullptr, &renderpass_) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass!");

  for (auto &fbo : swapchain_fbos_) {
    if (fbo != VK_NULL_HANDLE)
      vkDestroyFramebuffer(display_.device_, fbo, nullptr);
  }
  swapchain_fbos_.resize(swapchain_images_.size());
  for (size_t i = 0; i < swapchain_fbos_.size(); i++) {
    VkImageView attachments[] = {swapchain_imageviews_[i]};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderpass_;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapchain_extents_.width;
    framebufferInfo.height = swapchain_extents_.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(display_.device_, &framebufferInfo, nullptr, &swapchain_fbos_[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create framebuffer!");
  }

  if (!primary_cbs_.empty())
    vkFreeCommandBuffers(display_.device_, display_.commandg_pool_, primary_cbs_.size(), primary_cbs_.data());

  primary_cbs_.resize(images_count);
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = images_count;
  allocInfo.commandPool = display_.commandg_pool_;

  if (vkAllocateCommandBuffers(display_.device_, &allocInfo, primary_cbs_.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");

  if (sem_available_ != VK_NULL_HANDLE)
    vkDestroySemaphore(display_.device_, sem_available_, nullptr);
  if (sem_rendered_ != VK_NULL_HANDLE)
    vkDestroySemaphore(display_.device_, sem_rendered_, nullptr);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  if (vkCreateSemaphore(display_.device_, &semaphoreInfo, nullptr, &sem_available_) != VK_SUCCESS ||
      vkCreateSemaphore(display_.device_, &semaphoreInfo, nullptr, &sem_rendered_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create semaphores!");
  }

  for (size_t i = 0; i < dirty_.size(); i++)
    dirty_[i] = true;
  dirty_.resize(images_count, true);
}

void window::redraw(display::time_point _tp) {
  display_.check_thread();

  if (swapchain_ == VK_NULL_HANDLE)
    return;

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(display_.device_, swapchain_, std::numeric_limits<uint64_t>::max(),
                                          sem_available_, VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    init_vulkan_surface();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  on_frame.fire(size_, last_frame_ - _tp);
  display_.flush_staged();

  if (dirty_[imageIndex]) {
    vkDeviceWaitIdle(display_.device_);
    dirty_[imageIndex] = false;
    rebuild_cb(swapchain_fbos_[imageIndex], primary_cbs_[imageIndex]);
  }

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

  if (vkQueueSubmit(display_.queueg_, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
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

  result = vkQueuePresentKHR(display_.queuep_, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    init_vulkan_surface();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  auto present = display::clock::now();

  cbs_.clear();

  auto diff = present - last_frame_;
  if (fps_limit_ != 0) {
    auto limit = std::chrono::microseconds((uint)(1. / fps_limit_ * 1000 * 1000));
    if (diff < limit)
      std::this_thread::sleep_for(limit - diff);
  }
  else if (diff > 20ms) {
    std::cout << "[hut] frame took " << std::chrono::duration<double, std::milli>(diff).count() << "ms..." << std::endl;
  }

  last_frame_ = present;
}

void window::dispatch_resize(const uvec2 &_size) {
  size_ = _size;
  init_vulkan_surface();
  on_resize.fire(_size);
}

void window::rebuild_cb(VkFramebuffer _fbo, VkCommandBuffer _cb) {
  display_.check_thread();

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;  // Optional

  vkBeginCommandBuffer(_cb, &beginInfo);

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderpass_;
  renderPassInfo.framebuffer = _fbo;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapchain_extents_;

  VkClearValue clearColor = {{{clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(_cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);  // FIXME
  // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
  // to enable secondary command buffers

  on_draw.fire(_cb, size_);

  vkCmdEndRenderPass(_cb);
  if (vkEndCommandBuffer(_cb) != VK_SUCCESS)
    throw std::runtime_error("failed to record command buffer!");
}