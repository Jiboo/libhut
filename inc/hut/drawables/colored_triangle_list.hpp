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

#include <glm/glm.hpp>

#include "spv.h"

#include "hut/buffer.hpp"
#include "hut/color.hpp"
#include "hut/display.hpp"
#include "hut/window.hpp"

namespace hut {

class colored_triangle_list {
 public:
  struct vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription binding_desc() {
      VkVertexInputBindingDescription bindingDescription = {};
      bindingDescription.binding = 0;
      bindingDescription.stride = sizeof(vertex);
      bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      return bindingDescription;
    }
    static std::array<VkVertexInputAttributeDescription, 2> attributes_desc() {
      std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions =
          {};
      attributeDescriptions[0].binding = 0;
      attributeDescriptions[0].location = 0;
      attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
      attributeDescriptions[0].offset = offsetof(vertex, pos);
      attributeDescriptions[1].binding = 0;
      attributeDescriptions[1].location = 1;
      attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[1].offset = offsetof(vertex, color);
      return attributeDescriptions;
    }
  };

  window &window_;
  VkShaderModule vert_ = VK_NULL_HANDLE;
  VkShaderModule frag_ = VK_NULL_HANDLE;
  VkPipelineLayout layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;

  colored_triangle_list(window &_window) : window_(_window) {
    VkDevice device = _window.display_.device_;
    const glm::uvec2 &size = _window.size_;
    auto vertShaderCode = __spv::noubo_v2_pos_v3_color_vert_spv;
    auto fragShaderCode = __spv::noubo_v3_color_frag_spv;

    VkShaderModuleCreateInfo module_info = {};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = vertShaderCode.size();
    module_info.pCode = (uint32_t *)vertShaderCode.data();

    if (vkCreateShaderModule(device, &module_info, nullptr, &vert_) !=
        VK_SUCCESS)
      throw std::runtime_error("[sample] failed to create vertex module!");

    module_info.codeSize = fragShaderCode.size();
    module_info.pCode = (uint32_t *)fragShaderCode.data();

    if (vkCreateShaderModule(device, &module_info, nullptr, &frag_) !=
        VK_SUCCESS)
      throw std::runtime_error("[sample] failed to create fragment module!");

    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vert_stage_info,
                                                      frag_stage_info};

    auto bindingDescription = vertex::binding_desc();
    auto attributeDescriptions = vertex::attributes_desc();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount =
        (uint32_t)attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)size.x;
    viewport.height = (float)size.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {size.x, size.y};

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
    rasterizer.depthBiasClamp = 0.0f;           // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             /// Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ZERO;                                        // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor =
        VK_BLEND_FACTOR_ZERO;                             // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;  // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    const std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;          // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;       // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;  // Optional
    pipelineLayoutInfo.pPushConstantRanges = 0;     // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &layout_) != VK_SUCCESS)
      throw std::runtime_error("[sample] failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;  // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;  // Optional
    pipelineInfo.layout = layout_;
    pipelineInfo.renderPass = window_.renderpass_;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;  // Optional

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &pipeline_) != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline!");
    }
  }

  ~colored_triangle_list() {
    VkDevice device = window_.display_.device_;
    vkDeviceWaitIdle(device);
    if (pipeline_ != VK_NULL_HANDLE)
      vkDestroyPipeline(device, pipeline_, nullptr);
    if (layout_ != VK_NULL_HANDLE)
      vkDestroyPipelineLayout(device, layout_, nullptr);
    if (vert_ != VK_NULL_HANDLE) vkDestroyShaderModule(device, vert_, nullptr);
    if (frag_ != VK_NULL_HANDLE) vkDestroyShaderModule(device, frag_, nullptr);
  }

  template <typename T, uint32_t TCount>
  void draw(VkCommandBuffer _buffer, const glm::uvec2 &_size,
            const shared_ref<T, TCount> &_vertices) {
    window_.display_.check_thread();

    vkCmdBindPipeline(_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    VkBuffer vertexBuffers[] = {_vertices->buffer_.buffer_};
    VkDeviceSize offsets[] = {_vertices->offset_};
    vkCmdBindVertexBuffers(_buffer, 0, 1, vertexBuffers, offsets);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {_size.x, _size.y};
    vkCmdSetScissor(_buffer, 0, 1, &scissor);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_size.x;
    viewport.height = (float)_size.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(_buffer, 0, 1, &viewport);

    vkCmdDraw(_buffer, (uint32_t)TCount, 1, 0, 0);
  }
};

}  // namespace hut
