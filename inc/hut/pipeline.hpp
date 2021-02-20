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

#include "hut_shaders_refl.hpp"

#include "hut/buffer_pool.hpp"
#include "hut/color.hpp"
#include "hut/display.hpp"
#include "hut/image.hpp"
#include "hut/window.hpp"

namespace hut {

struct proj_ubo {
  mat4 proj_ {1};
};

struct pipeline_params {
  VkPrimitiveTopology topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkCompareOp depthCompare_ = VK_COMPARE_OP_LESS_OR_EQUAL;
  VkCullModeFlagBits cullMode_ = VK_CULL_MODE_NONE;
  VkFrontFace frontFace_ = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  VkBool32 enableBlending_ = VK_TRUE;
  uint32_t max_sets_ = 16;
};

template<typename TUBO, typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments>
class pipeline {
public:
  using ubo = TUBO;
  using indice = TIndice;
  using vertex = typename TVertexRefl::vertex;
  using instance = typename TVertexRefl::instance;

  using shared_ubo = shared_ref<ubo>;
  using shared_indices = shared_ref<indice>;
  using shared_vertices = shared_ref<vertex>;
  using shared_instances = shared_ref<instance>;

private:
  using extra_attachments = std::tuple<TExtraAttachments...>;

  static constexpr auto combined_bindings_ = combine(TVertexRefl::descriptor_bindings_, TFragRefl::descriptor_bindings_);

  struct attachments {
    shared_ubo ubo_buffer_;
    extra_attachments extras_;
  };

  window &window_;

  VkShaderModule vert_ = VK_NULL_HANDLE;
  VkShaderModule frag_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

  std::vector<VkDescriptorSet> descriptors_;
  std::unordered_map<uint, attachments> attachments_;

  void init_pools(const pipeline_params &_params) {
    std::array<VkDescriptorPoolSize, 2> descriptor_pools {
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 0},
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 0},
    };

    for (auto binding : combined_bindings_) {
      switch (binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: descriptor_pools[0].descriptorCount++; break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: descriptor_pools[1].descriptorCount++; break;
        default: assert(false);
      }
    }

    for (auto &pool : descriptor_pools)
      pool.descriptorCount *= _params.max_sets_;

    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = descriptor_pools[1].descriptorCount == 0 ? 1 : 2;
    create_info.pPoolSizes = descriptor_pools.data();
    create_info.maxSets = _params.max_sets_;

    if (vkCreateDescriptorPool(window_.display_.device_, &create_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor pool!");
  }

  void init_descriptor_layout() {
    VkDescriptorSetLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = combined_bindings_.size();
    create_info.pBindings = combined_bindings_.data();

    if (vkCreateDescriptorSetLayout(window_.display_.device_, &create_info, nullptr, &descriptor_layout_) != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor set layout!");
  }

  void init_shaders() {
    VkShaderModuleCreateInfo vert_create_info = {};
    vert_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_create_info.codeSize = TVertexRefl::bytecode_.size();
    vert_create_info.pCode = (uint32_t*)TVertexRefl::bytecode_.data();

    if (vkCreateShaderModule(window_.display_.device_, &vert_create_info, nullptr, &vert_) != VK_SUCCESS)
      throw std::runtime_error("[sample] failed to create vertex module!");

    VkShaderModuleCreateInfo frag_create_info = {};
    frag_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_create_info.codeSize = TFragRefl::bytecode_.size();
    frag_create_info.pCode = (uint32_t*)TFragRefl::bytecode_.data();

    if (vkCreateShaderModule(window_.display_.device_, &frag_create_info, nullptr, &frag_) != VK_SUCCESS)
      throw std::runtime_error("[sample] failed to create fragment module!");
  }

  void init_pipeline_layout() {
    VkDescriptorSetLayout set_layouts[] = {descriptor_layout_};
    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pushConstantRangeCount = 0;
    layout_create_info.pPushConstantRanges = nullptr;
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = set_layouts;

    if (vkCreatePipelineLayout(window_.display_.device_, &layout_create_info, nullptr, &layout_) != VK_SUCCESS)
      throw std::runtime_error("[sample] failed to create pipeline layout!");
  }

  void init_pipeline(uvec2 _default_size, VkSampleCountFlagBits _samples, const pipeline_params &_params) {
    HUT_PROFILE_SCOPE(PPIPELINE, __PRETTY_FUNCTION__);
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

    VkPipelineShaderStageCreateInfo shader_create_infos[] = {vert_stage_info, frag_stage_info};

    constexpr static std::array<VkVertexInputBindingDescription, 2> vertices_binding_ = {
        VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex),   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
    };

    VkPipelineVertexInputStateCreateInfo vertex_create_info = {};
    vertex_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_create_info.vertexBindingDescriptionCount = sizeof(instance) <= 1 ? 1 : 2;
    vertex_create_info.pVertexBindingDescriptions = vertices_binding_.data();
    vertex_create_info.vertexAttributeDescriptionCount = (uint32_t)TVertexRefl::vertices_description_.size();
    vertex_create_info.pVertexAttributeDescriptions = TVertexRefl::vertices_description_.data();

    VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
    assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_create_info.topology = _params.topology_;
    assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_default_size.x;
    viewport.height = (float)_default_size.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {_default_size.x, _default_size.y};

    VkPipelineViewportStateCreateInfo viewport_create_info = {};
    viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.pViewports = &viewport;
    viewport_create_info.scissorCount = 1;
    viewport_create_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {};
    rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_create_info.depthClampEnable = VK_FALSE;
    rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_create_info.lineWidth = 1.0f;
    rasterizer_create_info.cullMode = _params.cullMode_;
    rasterizer_create_info.frontFace = _params.frontFace_;
    rasterizer_create_info.depthBiasEnable = VK_FALSE;
    rasterizer_create_info.depthBiasConstantFactor = 0.0f;
    rasterizer_create_info.depthBiasClamp = 0.0f;
    rasterizer_create_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo msaa_create_info = {};
    msaa_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa_create_info.sampleShadingEnable = VK_FALSE;
    msaa_create_info.rasterizationSamples = _samples;
    msaa_create_info.minSampleShading = 1.0f;
    msaa_create_info.pSampleMask = nullptr;
    msaa_create_info.alphaToCoverageEnable = VK_FALSE;
    msaa_create_info.alphaToOneEnable = VK_FALSE;

    constexpr uint component_mask = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendAttachmentState blend_attachment = {};
    blend_attachment.colorWriteMask = component_mask;
    blend_attachment.blendEnable = _params.enableBlending_;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blend_create_info = {};
    blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_create_info.logicOpEnable = VK_FALSE;
    blend_create_info.logicOp = VK_LOGIC_OP_COPY;
    blend_create_info.attachmentCount = 1;
    blend_create_info.pAttachments = &blend_attachment;
    blend_create_info.blendConstants[0] = 0.0f;
    blend_create_info.blendConstants[1] = 0.0f;
    blend_create_info.blendConstants[2] = 0.0f;
    blend_create_info.blendConstants[3] = 0.0f;

    const std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_states_create_info = {};
    dynamic_states_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_states_create_info.dynamicStateCount = (uint32_t)dynamic_states.size();
    dynamic_states_create_info.pDynamicStates = dynamic_states.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = _params.depthCompare_;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shader_create_infos;
    pipelineInfo.pVertexInputState = &vertex_create_info;
    pipelineInfo.pInputAssemblyState = &assembly_create_info;
    pipelineInfo.pViewportState = &viewport_create_info;
    pipelineInfo.pRasterizationState = &rasterizer_create_info;
    pipelineInfo.pMultisampleState = &msaa_create_info;
    pipelineInfo.pColorBlendState = &blend_create_info;
    pipelineInfo.pDynamicState = &dynamic_states_create_info;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = layout_;
    pipelineInfo.renderPass = window_.renderpass_;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;  // Optional

    if (vkCreateGraphicsPipelines(window_.display_.device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline!");
  }

public:
  explicit pipeline(window &_window, const pipeline_params &_params = {})
      : window_(_window) {
    HUT_PROFILE_SCOPE(PPIPELINE, __PRETTY_FUNCTION__);
    init_pools(_params);
    init_descriptor_layout();
    alloc_next_descriptors(1);
    init_shaders();
    init_pipeline_layout();
    init_pipeline(_window.size(), _window.sample_count_, _params);
  }

  virtual ~pipeline() {
    VkDevice device = window_.display_.device_;
    HUT_PVK(vkDeviceWaitIdle, device);
    if (descriptor_layout_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyDescriptorSetLayout, device, descriptor_layout_, nullptr);
    if (descriptor_pool_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyDescriptorPool, device, descriptor_pool_, nullptr);
    if (pipeline_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyPipeline, device, pipeline_, nullptr);
    if (layout_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyPipelineLayout ,device, layout_, nullptr);
    if (vert_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyShaderModule, device, vert_, nullptr);
    if (frag_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyShaderModule, device, frag_, nullptr);
  }

  void alloc_next_descriptors(uint _count) {
    uint current_count = descriptors_.size();
    descriptors_.resize(current_count + _count);

    VkDescriptorSetLayout layouts[_count];
    for (uint i = 0; i < _count; i++)
      layouts[i] = descriptor_layout_;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = _count;
    alloc_info.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(window_.display_.device_, &alloc_info, descriptors_.data() + current_count) != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor set!");
  }

  struct descriptor_write_context {
    std::vector<VkWriteDescriptorSet> writes_;
    std::vector<VkDescriptorImageInfo> images_;
    std::vector<VkDescriptorBufferInfo> buffers_;
    VkDescriptorSet dst_ = VK_NULL_HANDLE;

    void buffer(uint _binding, const shared_ubo &_ubo) {
      VkDescriptorBufferInfo info = {};
      info.buffer = _ubo->alloc_.buffer_->buffer_;
      info.offset = _ubo->alloc_.offset_;
      info.range = _ubo->alloc_.size_;
      buffers_.emplace_back(info);

      VkWriteDescriptorSet write = {};
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = dst_;
      write.dstBinding = _binding;
      write.dstArrayElement = 0;
      write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write.descriptorCount = 1;
      write.pBufferInfo = &buffers_.back();
      writes_.emplace_back(write);
    }

    void texture(uint _binding, const shared_image &_mask, const shared_sampler &_sampler) {
      VkDescriptorImageInfo info = {};
      info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      info.imageView = _mask->view_;
      info.sampler = _sampler->sampler_;
      images_.emplace_back(info);

      VkWriteDescriptorSet write = {};
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = dst_;
      write.dstBinding = _binding;
      write.dstArrayElement = 0;
      write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount = 1;
      write.pImageInfo = &images_.back();
      writes_.emplace_back(write);
    }
  };

  void write_continue(int _binding, descriptor_write_context &_context) {}

  template<typename TBufferType, typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_ref<TBufferType> &_buffer, const TRest&... _rest) {
    _context.buffer(_binding);
    write_continue(_binding + 1, _context, std::forward<TRest>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_image &_image, const shared_sampler &_sampler, const TRest&... _rest) {
    _context.texture(_binding, _image, _sampler);
    write_continue(_binding + 1, _context, std::forward<TRest>(_rest)...);
  }

  void write(uint _descriptor_index, const shared_ubo &_ubo, const TExtraAttachments&... _attachments) {
    HUT_PROFILE_SCOPE(PPIPELINE, __PRETTY_FUNCTION__);
    assert(_descriptor_index < descriptors_.size());
    attachments_.emplace(_descriptor_index, attachments{_ubo, std::forward_as_tuple(_attachments...)});

    descriptor_write_context filler;
    filler.dst_ = *(descriptors_.data() + _descriptor_index);
    filler.buffer(0, _ubo);
    write_continue(1, filler, std::forward<TExtraAttachments>(_attachments)...);

    HUT_PVK(vkUpdateDescriptorSets, window_.display_.device_, filler.writes_.size(), filler.writes_.data(), 0, nullptr);
  }

  bool descriptor_attached(uint _descriptor_index) {
    return attachments_.find(_descriptor_index) != attachments_.end();
  }

  void bind_pipeline(VkCommandBuffer _buffer) {
    HUT_PVK(vkCmdBindPipeline, _buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  }

  void bind_descriptor(VkCommandBuffer _buffer, uint _descriptor_index) {
    assert(descriptor_attached(_descriptor_index));
    HUT_PVK(vkCmdBindDescriptorSets, _buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, 0, 1, descriptors_.data() + _descriptor_index, 0, nullptr);
  }

  void bind_vertices(VkCommandBuffer _buffer, const shared_vertices &_vertices, uint _vertex_offset) {
    assert(_vertices);
    assert(_vertex_offset <= _vertices->size());
    VkBuffer verticesBuffers[] = {_vertices->buff().buffer_};
    VkDeviceSize verticesOffsets[] = {_vertices->alloc_.offset_};
    HUT_PVK(vkCmdBindVertexBuffers, _buffer, 0, 1, verticesBuffers, verticesOffsets);
  }

  void bind_instances(VkCommandBuffer _buffer, const shared_instances &_instances, uint _instances_offset, uint _instances_count) {
    if constexpr (sizeof(instance) > 1) {
      assert(_instances);
      assert(_instances_offset + _instances_count <= _instances->size());
      VkBuffer instancesBuffers[] = {_instances->buff().buffer_};
      VkDeviceSize instancesOffsets[] = {_instances->alloc_.offset_};
      HUT_PVK(vkCmdBindVertexBuffers, _buffer, 1, 1, instancesBuffers, instancesOffsets);
    }
  }

  void bind_indices(VkCommandBuffer _buffer, const shared_indices &_indices, uint _indices_offset, uint _indices_count) {
    assert(_indices);
    assert(_indices_offset + _indices_count <= _indices->size());
    HUT_PVK(vkCmdBindIndexBuffer, _buffer, _indices->buff().buffer_, _indices->alloc_.offset_, VK_INDEX_TYPE_UINT16);
  }

  void draw_indexed(VkCommandBuffer _buffer, uint _indices_count, uint _instances_count, uint _indices_offset, uint _vertex_offset, uint _instances_offset)  {
    HUT_PVK(vkCmdDrawIndexed, _buffer, _indices_count, _instances_count, _indices_offset, _vertex_offset, _instances_offset);
  }

  void draw(VkCommandBuffer _buffer, uint _descriptor_index,
            const shared_indices &_indices, uint _indices_offset, uint _indices_count,
            const shared_instances &_instances, uint _instances_offset, uint _instances_count,
            const shared_vertices &_vertices, uint _vertex_offset) {
    HUT_PROFILE_SCOPE(PPIPELINE, __PRETTY_FUNCTION__);

    bind_pipeline(_buffer);
    bind_descriptor(_buffer, _descriptor_index);
    bind_vertices(_buffer, _vertices, _vertex_offset);
    bind_instances(_buffer, _instances, _instances_offset, _instances_count);
    bind_indices(_buffer, _indices, _indices_offset, _indices_count);
    draw_indexed(_buffer, _indices_count, _instances_count, _indices_offset, _vertex_offset, _instances_offset);
  }

  void draw(VkCommandBuffer _buffer, uint _descriptor_index,
            const shared_indices &_indices,
            const shared_instances &_instances,
            const shared_vertices &_vertices) {
    draw(_buffer, _descriptor_index,
        _indices, 0, _indices->size(),
        _instances, 0, _instances ? _instances->size() : 1,
        _vertices, 0);
  }

  void draw(VkCommandBuffer _buffer, uint _descriptor_index,
            const shared_indices &_indices, uint _indices_count,
            const shared_instances &_instances,
            const shared_vertices &_vertices) {
    draw(_buffer, _descriptor_index,
         _indices, 0, _indices_count,
         _instances, 0, _instances ? _instances->size() : 1,
         _vertices, 0);
  }
};

using rgb      = pipeline<proj_ubo, uint16_t, hut_shaders::rgb_vert_spv_refl, hut_shaders::rgb_frag_spv_refl>;
using rgba     = pipeline<proj_ubo, uint16_t, hut_shaders::rgba_vert_spv_refl, hut_shaders::rgba_frag_spv_refl>;
using tex      = pipeline<proj_ubo, uint16_t, hut_shaders::tex_vert_spv_refl, hut_shaders::tex_frag_spv_refl, const shared_image &, const shared_sampler &>;
using tex_rgb  = pipeline<proj_ubo, uint16_t, hut_shaders::tex_rgb_vert_spv_refl, hut_shaders::tex_rgb_frag_spv_refl, const shared_image &, const shared_sampler &>;
using tex_rgba = pipeline<proj_ubo, uint16_t, hut_shaders::tex_rgba_vert_spv_refl, hut_shaders::tex_rgba_frag_spv_refl, const shared_image &, const shared_sampler &>;
using tex_mask = pipeline<proj_ubo, uint16_t, hut_shaders::tex_mask_vert_spv_refl, hut_shaders::tex_mask_frag_spv_refl, const shared_image &, const shared_sampler &>;
using box_rgba = pipeline<proj_ubo, uint16_t, hut_shaders::box_rgba_vert_spv_refl, hut_shaders::box_rgba_frag_spv_refl>;

}  // namespace hut
