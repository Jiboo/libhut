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

#include "hut/color.hpp"
#include "hut/display.hpp"
#include "hut/image.hpp"
#include "hut/profiling.hpp"
#include "hut/render_target.hpp"

using namespace hut;

render_target::render_target(display &_display)
    : display_(_display) {
  HUT_PROFILE_SCOPE(PWINDOW, "render_target::render_target");
}

render_target::~render_target() {
  HUT_PROFILE_SCOPE(PWINDOW, "render_target::~render_target");

  for (auto &fbo : fbos_) {
    if (fbo != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyFramebuffer, display_.device_, fbo, nullptr);
  }

  if (renderpass_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroyRenderPass, display_.device_, renderpass_, nullptr);
}

void render_target::reinit_pass(const render_target_params &_init_params, std::span<VkImageView> _images) {
  HUT_PROFILE_SCOPE(PWINDOW, "render_target::reinit");
  render_target_params_ = _init_params;
  const auto size = bbox_size(_init_params.box_);
  const auto offset = bbox_origin(_init_params.box_);

  assert(size.x > 0 && size.y > 0);

  if (render_target_params_.flags_ & render_target_params::FMULTISAMPLING) {
    VkSampleCountFlags counts = display_.limits().framebufferColorSampleCounts & display_.limits().framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_8_BIT) sample_count_ = VK_SAMPLE_COUNT_8_BIT;
    else if (counts & VK_SAMPLE_COUNT_4_BIT) sample_count_ = VK_SAMPLE_COUNT_4_BIT;
    else if (counts & VK_SAMPLE_COUNT_2_BIT) sample_count_ = VK_SAMPLE_COUNT_2_BIT;

    if (sample_count_ != VK_SAMPLE_COUNT_1_BIT) {
      image_params params;
      params.size_ = size;
      params.format_ = _init_params.format_;
      params.tiling_ = VK_IMAGE_TILING_OPTIMAL;
      params.usage_ = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      params.aspect_ = VK_IMAGE_ASPECT_COLOR_BIT;
      params.samples_ = sample_count_;
      msaa_rendertarget_ = std::make_shared<image>(display_, params);
    }
  }

  VkFormat depth_format = VK_FORMAT_UNDEFINED;
  const VkPhysicalDevice &pdevice = display_.pdevice_;
  if (render_target_params_.flags_ & render_target_params::FDEPTH) {
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

    image_params params;
    params.size_ = size;
    params.format_ = depth_format;
    params.tiling_ = VK_IMAGE_TILING_OPTIMAL;
    params.usage_ = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    params.aspect_ = VK_IMAGE_ASPECT_DEPTH_BIT;
    params.samples_ = sample_count_;
    depth_ = std::make_shared<image>(display_, params);
  }

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = _init_params.format_;
  colorAttachment.samples = sample_count_;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = _init_params.initial_layout_;
  colorAttachment.finalLayout = msaa_rendertarget_ ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : _init_params.final_layout_;

  std::vector<VkAttachmentDescription> passAttachments {colorAttachment};

  if (depth_) {
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depth_format;
    depthAttachment.samples = sample_count_;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    passAttachments.emplace_back(depthAttachment);
  }

  if (msaa_rendertarget_) {
    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = _init_params.format_;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = _init_params.initial_layout_;
    colorAttachmentResolve.finalLayout = _init_params.final_layout_;

    passAttachments.emplace_back(colorAttachmentResolve);
  }

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentResolveRef{};
  colorAttachmentResolveRef.attachment = depth_ ? 2 : 1;
  colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = depth_ ? &depthAttachmentRef : nullptr;
  subpass.pResolveAttachments = msaa_rendertarget_ ? &colorAttachmentResolveRef : nullptr;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = passAttachments.size();
  renderPassInfo.pAttachments = passAttachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (renderpass_ != VK_NULL_HANDLE)
    HUT_PVK(vkDestroyRenderPass, display_.device_, renderpass_, nullptr);
  if (HUT_PVK(vkCreateRenderPass, display_.device_, &renderPassInfo, nullptr, &renderpass_) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass!");

  for (auto &fbo : fbos_) {
    if (fbo != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyFramebuffer, display_.device_, fbo, nullptr);
  }
  fbos_.clear();
  fbos_.resize(_images.size());

  std::vector<VkImageView> fbAttachments;
  for (size_t i = 0; i < fbos_.size(); i++) {
    fbAttachments.clear();

    if (msaa_rendertarget_)
      fbAttachments.emplace_back(msaa_rendertarget_->view_);
    else
      fbAttachments.emplace_back(_images[i]);

    if (depth_)
      fbAttachments.emplace_back(depth_->view_);

    if (msaa_rendertarget_)
      fbAttachments.emplace_back(_images[i]);

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderpass_;
    framebufferInfo.attachmentCount = fbAttachments.size();
    framebufferInfo.pAttachments = fbAttachments.data();
    framebufferInfo.width = offset.x + size.x;
    framebufferInfo.height = offset.y + size.y;
    framebufferInfo.layers = 1;

    if (HUT_PVK(vkCreateFramebuffer, display_.device_, &framebufferInfo, nullptr, &fbos_[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create framebuffer!");
  }
}

void render_target::begin_rebuild_cb(VkFramebuffer _fbo, VkCommandBuffer _cb) {
  HUT_PROFILE_SCOPE(PWINDOW, "window::rebuild_cb");
  auto offset = bbox_origin(render_target_params_.box_);
  auto size = bbox_size(render_target_params_.box_);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;  // Optional

  HUT_PVK(vkBeginCommandBuffer, _cb, &beginInfo);

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderpass_;
  renderPassInfo.framebuffer = _fbo;
  renderPassInfo.renderArea.offset = {offset.x, offset.y};
  renderPassInfo.renderArea.extent = {size.x, size.y};

  std::vector<VkClearValue> clearColors;
  clearColors.emplace_back(VkClearValue{.color = render_target_params_.clear_color_});
  if (depth_)
    clearColors.emplace_back(VkClearValue{.depthStencil = render_target_params_.clear_depth_stencil_});
  if (msaa_rendertarget_)
    clearColors.emplace_back(VkClearValue{.color = render_target_params_.clear_color_});

  renderPassInfo.clearValueCount = clearColors.size();
  renderPassInfo.pClearValues = clearColors.data();

  HUT_PVK(vkCmdBeginRenderPass, _cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);  // FIXME
  // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
  // to enable secondary command buffers

  VkRect2D scissor = {};
  scissor.offset = {offset.x, offset.y};
  scissor.extent = {size.x, size.y};
  HUT_PVK(vkCmdSetScissor, _cb, 0, 1, &scissor);

  VkViewport viewport = {};
  viewport.x = offset.x;
  viewport.y = offset.y;
  viewport.width = size.x;
  viewport.height = size.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  HUT_PVK(vkCmdSetViewport, _cb, 0, 1, &viewport);
}

void render_target::end_rebuild_cb(VkCommandBuffer _cb) {
  HUT_PVK(vkCmdEndRenderPass, _cb);
  if (HUT_PVK(vkEndCommandBuffer, _cb) != VK_SUCCESS)
    throw std::runtime_error("failed to record command buffer!");
}
