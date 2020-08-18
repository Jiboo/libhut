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

#include "hut_shaders.hpp"

#include "hut/buffer_pool.hpp"
#include "hut/color.hpp"
#include "hut/display.hpp"
#include "hut/image.hpp"
#include "hut/window.hpp"

namespace hut {

struct proj_ubo {
  mat4 proj_ {1};
};

template<typename T>
using shared_ubo = shared_ref<T>;

template<typename T>
static shared_ubo<T> alloc_ubo(const display &_display, shared_buffer &_buf, const T &_default) {
  auto ref = _buf->allocate<T>(1, _display.limits().minUniformBufferOffsetAlignment);
  ref->set(_default);
  return ref;
}

template<typename TDetails, typename... TExtraBindings>
class drawable {
public:
  using instance = typename TDetails::instance;
  using vertex = typename TDetails::vertex;
  using ubo = typename TDetails::ubo;
  using indice = uint16_t;

  using shared_instances = shared_ref<instance>;
  using shared_vertices = shared_ref<vertex>;
  using shared_indices = shared_ref<indice>;

private:
  using extra_bindings = std::tuple<TExtraBindings...>;

  struct bindings {
    shared_ubo<ubo> ubo_buffer_;
    extra_bindings extras_;
  };

  window &window_;

  VkShaderModule vert_ = VK_NULL_HANDLE;
  VkShaderModule frag_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

  std::vector<VkDescriptorSet> descriptors_;
  std::unordered_map<uint, bindings> bindings_;

  void init_pools() {
    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = TDetails::descriptor_pools_.size();
    create_info.pPoolSizes = TDetails::descriptor_pools_.data();
    create_info.maxSets = TDetails::max_descriptor_sets_;

    if (vkCreateDescriptorPool(window_.display_.device_, &create_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor pool!");
  }

  void init_descriptor_layout() {
    VkDescriptorSetLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = TDetails::descriptor_bindings_.size();
    create_info.pBindings = TDetails::descriptor_bindings_.data();

    if (vkCreateDescriptorSetLayout(window_.display_.device_, &create_info, nullptr, &descriptor_layout_) != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor set layout!");
  }

  void init_shaders() {
    VkShaderModuleCreateInfo vert_create_info = {};
    vert_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_create_info.codeSize = TDetails::vert_bytecode_.size();
    vert_create_info.pCode = (uint32_t*)TDetails::vert_bytecode_.data();

    if (vkCreateShaderModule(window_.display_.device_, &vert_create_info, nullptr, &vert_) != VK_SUCCESS)
      throw std::runtime_error("[sample] failed to create vertex module!");

    VkShaderModuleCreateInfo frag_create_info = {};
    frag_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_create_info.codeSize = TDetails::frag_bytecode_.size();
    frag_create_info.pCode = (uint32_t*)TDetails::frag_bytecode_.data();

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

  void init_pipeline(uvec2 _default_size, VkPrimitiveTopology _topology
#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
      , VkCompareOp _depthCompare
#endif
    ) {
    HUT_PROFILE_SCOPE(PDRAWABLE, __PRETTY_FUNCTION__);
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

    VkPipelineVertexInputStateCreateInfo vertex_create_info = {};
    vertex_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_create_info.vertexBindingDescriptionCount = uint(TDetails::vertices_binding_.size());
    vertex_create_info.pVertexBindingDescriptions = TDetails::vertices_binding_.data();
    vertex_create_info.vertexAttributeDescriptionCount = (uint32_t)TDetails::vertices_description_.size();
    vertex_create_info.pVertexAttributeDescriptions = TDetails::vertices_description_.data();

    VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
    assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_create_info.topology = _topology;
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
    rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_create_info.depthBiasEnable = VK_FALSE;
    rasterizer_create_info.depthBiasConstantFactor = 0.0f;
    rasterizer_create_info.depthBiasClamp = 0.0f;
    rasterizer_create_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo msaa_create_info = {};
    msaa_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa_create_info.sampleShadingEnable = VK_FALSE;
    msaa_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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
    blend_attachment.blendEnable = VK_TRUE;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
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
    pipelineInfo.layout = layout_;
    pipelineInfo.renderPass = window_.renderpass_;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;  // Optional

#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = _depthCompare;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    pipelineInfo.pDepthStencilState = _depthCompare == VK_COMPARE_OP_NEVER ? nullptr : &depthStencil;
#endif

    if (vkCreateGraphicsPipelines(window_.display_.device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline!");
  }

public:
  explicit drawable(window &_window, VkPrimitiveTopology _topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
    , VkCompareOp _depthCompare = VK_COMPARE_OP_LESS
#endif
      )
      : window_(_window) {
    HUT_PROFILE_SCOPE(PDRAWABLE, __PRETTY_FUNCTION__);
    init_pools();
    init_descriptor_layout();
    alloc_next_descriptors(1);
    init_shaders();
    init_pipeline_layout();
    init_pipeline(_window.size(), _topology
#ifdef HUT_ENABLE_WINDOW_DEPTH_BUFFER
        , _depthCompare
#endif
    );
  }

  virtual ~drawable() {
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

    void buffer(uint _binding, const shared_ubo<ubo> &_ubo) {
      VkDescriptorBufferInfo info = {};
      info.buffer = _ubo->buffer_;
      info.offset = _ubo->offset_;
      info.range = _ubo->byte_size_;
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

  void bind(uint _descriptor_index, const shared_ubo<ubo> &_ubo, TExtraBindings... _bindings) {
    HUT_PROFILE_SCOPE(PDRAWABLE, __PRETTY_FUNCTION__);
    assert(_descriptor_index < descriptors_.size());
    bindings_.emplace(_descriptor_index, bindings{_ubo, std::forward_as_tuple(_bindings...)});

    descriptor_write_context filler;
    filler.dst_ = *(descriptors_.data() + _descriptor_index);
    filler.buffer(0, _ubo);
    TDetails::fill_extra_descriptor_writes(filler, std::forward<TExtraBindings>(_bindings)...);

    HUT_PVK(vkUpdateDescriptorSets, window_.display_.device_, filler.writes_.size(), filler.writes_.data(), 0, nullptr);
  }

  bool descriptor_bound(uint _descriptor_index) {
    return bindings_.find(_descriptor_index) != bindings_.end();
  }

  void draw(VkCommandBuffer _buffer, uint _descriptor_index,
            const shared_indices &_indices, uint _indices_offset, uint _indices_count,
            const shared_instances &_instances, uint _instances_offset, uint _instances_count,
            const shared_vertices &_vertices, uint _vertex_offset) {
    HUT_PROFILE_SCOPE(PDRAWABLE, __PRETTY_FUNCTION__);
    assert(descriptor_bound(_descriptor_index));
    assert(_indices && _instances && _vertices);
    assert(_indices_offset + _indices_count <= _indices->size());
    assert(_instances_offset + _instances_count <= _instances->size());
    assert(_vertex_offset <= _vertices->size());

    HUT_PVK(vkCmdBindPipeline, _buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    HUT_PVK(vkCmdBindDescriptorSets, _buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, 0, 1, descriptors_.data() + _descriptor_index, 0, nullptr);

    VkBuffer verticesBuffers[] = {_vertices->buffer_};
    VkDeviceSize verticesOffsets[] = {_vertices->offset_};
    HUT_PVK(vkCmdBindVertexBuffers, _buffer, 0, 1, verticesBuffers, verticesOffsets);

    VkBuffer instancesBuffers[] = {_instances->buffer_};
    VkDeviceSize instancesOffsets[] = {_instances->offset_};
    HUT_PVK(vkCmdBindVertexBuffers, _buffer, 1, 1, instancesBuffers, instancesOffsets);

    HUT_PVK(vkCmdBindIndexBuffer, _buffer, _indices->buffer_, _indices->offset_, VK_INDEX_TYPE_UINT16);
    HUT_PVK(vkCmdDrawIndexed, _buffer, _indices_count, _instances_count, _indices_offset, _vertex_offset, _instances_offset);
  }

  void draw(VkCommandBuffer _buffer, uint _descriptor_index,
            const shared_indices &_indices,
            const shared_instances &_instances,
            const shared_vertices &_vertices) {
    assert(_indices && _instances);
    draw(_buffer, _descriptor_index,
        _indices, 0, _indices->size(),
        _instances, 0, _instances->size(),
        _vertices, 0);
  }

  void draw(VkCommandBuffer _buffer, uint _descriptor_index,
            const shared_indices &_indices, uint _indices_count,
            const shared_instances &_instances,
            const shared_vertices &_vertices) {
    assert(_indices && _instances);
    draw(_buffer, _descriptor_index,
         _indices, 0, _indices_count,
         _instances, 0, _instances->size(),
         _vertices, 0);
  }
};

namespace details {

  template<typename T>
  constexpr uint transform_offset(uint _column) {
    return offsetof(typename T::instance, transform_) + sizeof(vec4) * _column;
  }

  using descriptor_writes = std::vector<VkWriteDescriptorSet>;

  // TODO JBL: Generate this from reflection on shader code at compile time, and include generated .h

  struct rgb {
    using impl = drawable<details::rgb>;
    using ubo = proj_ubo;

    struct instance {
      mat4 transform_;
    };

    struct vertex {
      vec2 pos_;
      vec3 color_;
    };

    constexpr static uint max_descriptor_sets_ = 1;

    constexpr static std::array<VkDescriptorPoolSize, 1> descriptor_pools_ {
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = max_descriptor_sets_},
    };

    constexpr static std::array<VkDescriptorSetLayoutBinding, 1> descriptor_bindings_{
        VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .pImmutableSamplers = nullptr},
    };

    static void fill_extra_descriptor_writes(impl::descriptor_write_context &) {
    }

    constexpr static std::array<VkVertexInputBindingDescription, 2> vertices_binding_ = {
        VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex),   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
    };

    constexpr static std::array<VkVertexInputAttributeDescription, 6> vertices_description_ {
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, pos_)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(vertex, color_)},
        VkVertexInputAttributeDescription{.location = 2, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(0)},
        VkVertexInputAttributeDescription{.location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(1)},
        VkVertexInputAttributeDescription{.location = 4, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(2)},
        VkVertexInputAttributeDescription{.location = 5, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(3)},
    };

    constexpr static decltype(hut_shaders::rgb_frag_spv) &frag_bytecode_ = hut_shaders::rgb_frag_spv;
    constexpr static decltype(hut_shaders::rgb_vert_spv) &vert_bytecode_ = hut_shaders::rgb_vert_spv;
  };

  struct rgba {
    using impl = drawable<details::rgba>;
    using ubo = proj_ubo;

    struct instance {
      mat4 transform_;
    };

    struct vertex {
      vec2 pos_;
      vec4 color_;
    };

    constexpr static uint max_descriptor_sets_ = 1;

    constexpr static std::array<VkDescriptorPoolSize, 1> descriptor_pools_ {
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = max_descriptor_sets_},
    };

    constexpr static std::array<VkDescriptorSetLayoutBinding, 1> descriptor_bindings_{
        VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .pImmutableSamplers = nullptr},
    };

    static void fill_extra_descriptor_writes(impl::descriptor_write_context &) {}

    constexpr static std::array<VkVertexInputBindingDescription, 2> vertices_binding_ = {
        VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex),   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
    };

    constexpr static std::array<VkVertexInputAttributeDescription, 6> vertices_description_ {
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, pos_)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(vertex, color_)},
        VkVertexInputAttributeDescription{.location = 2, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(0)},
        VkVertexInputAttributeDescription{.location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(1)},
        VkVertexInputAttributeDescription{.location = 4, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(2)},
        VkVertexInputAttributeDescription{.location = 5, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<rgb>(3)},
    };

    constexpr static decltype(hut_shaders::rgba_frag_spv) &frag_bytecode_ = hut_shaders::rgba_frag_spv;
    constexpr static decltype(hut_shaders::rgba_vert_spv) &vert_bytecode_ = hut_shaders::rgba_vert_spv;
  };

  struct tex {
    using impl = drawable<details::tex, const shared_image &, const shared_sampler &>;
    using ubo = proj_ubo;

    struct instance {
      mat4 transform_;
    };

    struct vertex {
      vec2 pos_;
      vec2 texcoords_;
    };

    constexpr static uint max_descriptor_sets_ = 128;

    constexpr static std::array<VkDescriptorPoolSize, 2> descriptor_pools_ {
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = max_descriptor_sets_},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = max_descriptor_sets_},
    };

    constexpr static std::array<VkDescriptorSetLayoutBinding, 2> descriptor_bindings_{
        VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .pImmutableSamplers = nullptr},
        VkDescriptorSetLayoutBinding{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = nullptr},
    };

    static void fill_extra_descriptor_writes(impl::descriptor_write_context &_context,
                                                       const shared_image &_mask, const shared_sampler &_sampler) {
      _context.texture(1, _mask, _sampler);
    }

    constexpr static std::array<VkVertexInputBindingDescription, 2> vertices_binding_ = {
        VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex),   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
    };

    constexpr static std::array<VkVertexInputAttributeDescription, 6> vertices_description_ {
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, pos_)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, texcoords_)},
        VkVertexInputAttributeDescription{.location = 2, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(0)},
        VkVertexInputAttributeDescription{.location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(1)},
        VkVertexInputAttributeDescription{.location = 4, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(2)},
        VkVertexInputAttributeDescription{.location = 5, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(3)},
    };

    constexpr static decltype(hut_shaders::tex_frag_spv) &frag_bytecode_ = hut_shaders::tex_frag_spv;
    constexpr static decltype(hut_shaders::tex_vert_spv) &vert_bytecode_ = hut_shaders::tex_vert_spv;
  };

  struct tex_rgb {
    using impl = drawable<details::tex_rgb, const shared_image &, const shared_sampler &>;
    using ubo = proj_ubo;

    struct instance {
      mat4 transform_;
    };

    struct vertex {
      vec2 pos_;
      vec2 texcoords_;
      vec3 color_;
    };

    constexpr static uint max_descriptor_sets_ = 128;

    constexpr static std::array<VkDescriptorPoolSize, 2> descriptor_pools_ {
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = max_descriptor_sets_},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = max_descriptor_sets_},
    };

    constexpr static std::array<VkDescriptorSetLayoutBinding, 2> descriptor_bindings_{
        VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .pImmutableSamplers = nullptr},
        VkDescriptorSetLayoutBinding{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = nullptr},
    };

    static void fill_extra_descriptor_writes(impl::descriptor_write_context &_context,
                                                       const shared_image &_mask, const shared_sampler &_sampler) {
      _context.texture(1, _mask, _sampler);
    }

    constexpr static std::array<VkVertexInputBindingDescription, 2> vertices_binding_ = {
        VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex),   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
    };

    constexpr static std::array<VkVertexInputAttributeDescription, 7> vertices_description_ {
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, pos_)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, texcoords_)},
        VkVertexInputAttributeDescription{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(vertex, color_)},
        VkVertexInputAttributeDescription{.location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(0)},
        VkVertexInputAttributeDescription{.location = 4, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(1)},
        VkVertexInputAttributeDescription{.location = 5, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(2)},
        VkVertexInputAttributeDescription{.location = 6, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(3)},
    };

    constexpr static decltype(hut_shaders::tex_rgb_frag_spv) &frag_bytecode_ = hut_shaders::tex_rgb_frag_spv;
    constexpr static decltype(hut_shaders::tex_rgb_vert_spv) &vert_bytecode_ = hut_shaders::tex_rgb_vert_spv;
  };

  struct tex_rgba {
    using impl = drawable<details::tex_rgba, const shared_image &, const shared_sampler &>;
    using ubo = proj_ubo;

    struct instance {
      mat4 transform_;
    };

    struct vertex {
      vec2 pos_;
      vec2 texcoords_;
      vec4 color_;
    };

    constexpr static uint max_descriptor_sets_ = 128;

    constexpr static std::array<VkDescriptorPoolSize, 2> descriptor_pools_ {
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = max_descriptor_sets_},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = max_descriptor_sets_},
    };

    constexpr static std::array<VkDescriptorSetLayoutBinding, 2> descriptor_bindings_{
        VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .pImmutableSamplers = nullptr},
        VkDescriptorSetLayoutBinding{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = nullptr},
    };

    static void fill_extra_descriptor_writes(impl::descriptor_write_context &_context,
                                                       const shared_image &_mask, const shared_sampler &_sampler) {
      _context.texture(1, _mask, _sampler);
    }

    constexpr static std::array<VkVertexInputBindingDescription, 2> vertices_binding_ = {
        VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex),   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
    };

    constexpr static std::array<VkVertexInputAttributeDescription, 7> vertices_description_ {
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, pos_)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, texcoords_)},
        VkVertexInputAttributeDescription{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(vertex, color_)},
        VkVertexInputAttributeDescription{.location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(0)},
        VkVertexInputAttributeDescription{.location = 4, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(1)},
        VkVertexInputAttributeDescription{.location = 5, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(2)},
        VkVertexInputAttributeDescription{.location = 6, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex>(3)},
    };

    constexpr static decltype(hut_shaders::tex_rgba_frag_spv) &frag_bytecode_ = hut_shaders::tex_rgba_frag_spv;
    constexpr static decltype(hut_shaders::tex_rgba_vert_spv) &vert_bytecode_ = hut_shaders::tex_rgba_vert_spv;
  };

  struct tex_mask {
    using impl = drawable<details::tex_mask, const shared_image &, const shared_sampler &>;
    using ubo = proj_ubo;

    struct instance {
      mat4 transform_;
      vec4 color_;
    };

    struct vertex {
      vec2 pos_;
      vec2 texcoords_;
    };

    constexpr static uint max_descriptor_sets_ = 128;

    constexpr static std::array<VkDescriptorPoolSize, 2> descriptor_pools_ {
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = max_descriptor_sets_},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = max_descriptor_sets_},
    };

    constexpr static std::array<VkDescriptorSetLayoutBinding, 2> descriptor_bindings_{
        VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .pImmutableSamplers = nullptr},
        VkDescriptorSetLayoutBinding{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = nullptr},
    };

    static void fill_extra_descriptor_writes(impl::descriptor_write_context &_context,
                                                       const shared_image &_mask, const shared_sampler &_sampler) {
      _context.texture(1, _mask, _sampler);
    }

    constexpr static std::array<VkVertexInputBindingDescription, 2> vertices_binding_ = {
        VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex),   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
    };

    constexpr static std::array<VkVertexInputAttributeDescription, 7> vertices_description_ {
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, pos_)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, texcoords_)},
        VkVertexInputAttributeDescription{.location = 2, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex_mask>(0)},
        VkVertexInputAttributeDescription{.location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex_mask>(1)},
        VkVertexInputAttributeDescription{.location = 4, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex_mask>(2)},
        VkVertexInputAttributeDescription{.location = 5, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex_mask>(3)},
        VkVertexInputAttributeDescription{.location = 6, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(instance, color_)},
    };

    constexpr static decltype(hut_shaders::tex_mask_frag_spv) &frag_bytecode_ = hut_shaders::tex_mask_frag_spv;
    constexpr static decltype(hut_shaders::tex_mask_vert_spv) &vert_bytecode_ = hut_shaders::tex_mask_vert_spv;
  };

  struct box_rgba {
    using impl = drawable<details::box_rgba>;
    using ubo = proj_ubo;

    struct instance {
      mat4 transform_;
      vec2 params_;
      vec4 box_bounds_;
    };

    struct vertex {
      vec2 pos_;
      vec4 color_;
    };

    constexpr static uint max_descriptor_sets_ = 1;

    constexpr static std::array<VkDescriptorPoolSize, 1> descriptor_pools_ {
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = max_descriptor_sets_},
    };

    constexpr static std::array<VkDescriptorSetLayoutBinding, 1> descriptor_bindings_{
        VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .pImmutableSamplers = nullptr},
    };

    static void fill_extra_descriptor_writes(impl::descriptor_write_context &) {}

    constexpr static std::array<VkVertexInputBindingDescription, 2> vertices_binding_ = {
        VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex),   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
    };

    constexpr static std::array<VkVertexInputAttributeDescription, 8> vertices_description_ {
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT,       .offset = offsetof(vertex, pos_)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(vertex, color_)},
        VkVertexInputAttributeDescription{.location = 2, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex_mask>(0)},
        VkVertexInputAttributeDescription{.location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex_mask>(1)},
        VkVertexInputAttributeDescription{.location = 4, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex_mask>(2)},
        VkVertexInputAttributeDescription{.location = 5, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = transform_offset<tex_mask>(3)},
        VkVertexInputAttributeDescription{.location = 6, .binding = 1, .format = VK_FORMAT_R32G32_SFLOAT,       .offset = offsetof(instance, params_)},
        VkVertexInputAttributeDescription{.location = 7, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(instance, box_bounds_)},
    };

    constexpr static decltype(hut_shaders::box_rgba_frag_spv) &frag_bytecode_ = hut_shaders::box_rgba_frag_spv;
    constexpr static decltype(hut_shaders::box_rgba_vert_spv) &vert_bytecode_ = hut_shaders::box_rgba_vert_spv;
  };
}  // namespace details

using rgb      = details::rgb::impl;
using rgba     = details::rgba::impl;
using tex      = details::tex::impl;
using tex_rgb  = details::tex_rgb::impl;
using tex_rgba = details::tex_rgba::impl;
using tex_mask = details::tex_mask::impl;
using box_rgba = details::box_rgba::impl;

}  // namespace hut
