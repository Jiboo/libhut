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

#include "hut/atlas_pool.hpp"
#include "hut/buffer_pool.hpp"
#include "hut/color.hpp"
#include "hut/display.hpp"
#include "hut/image.hpp"
#include "hut/profiling.hpp"
#include "hut/window.hpp"

namespace hut {

struct pipeline_params {
  VkPrimitiveTopology topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkPolygonMode polygonMode_ = VK_POLYGON_MODE_FILL;
  VkCompareOp depthCompare_ = VK_COMPARE_OP_LESS_OR_EQUAL;
  VkCullModeFlagBits cullMode_ = VK_CULL_MODE_NONE;
  VkFrontFace frontFace_ = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  VkBool32 enableBlending_ = VK_TRUE;
  uint32_t max_sets_ = 16;
};

template<typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments>
class pipeline {
public:
  using indice = TIndice;
  using vertex = typename TVertexRefl::vertex;
  using instance = typename TVertexRefl::instance;

  using shared_indices = shared_ref<indice>;
  using shared_vertices = shared_ref<vertex>;
  using shared_instances = shared_ref<instance>;

private:
  using extra_attachments = std::tuple<TExtraAttachments...>;

  struct attachments {
    extra_attachments extras_;
  };

  VkDevice device_ref_;
  VkRenderPass render_pass_;

  VkShaderModule vert_ = VK_NULL_HANDLE;
  VkShaderModule frag_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

  std::vector<VkDescriptorSetLayoutBinding> bindings_;
  std::vector<VkDescriptorSet> descriptors_;
  std::unordered_map<uint, attachments> attachments_;

  struct per_descriptor_atlas_info {
    shared_sampler sampler_;
    uint last_bound_ = 0;
  };
  struct per_atlas_info {
    std::unordered_map<uint, per_descriptor_atlas_info> descriptors_info_;
    uint binding_;
  };
  std::unordered_map<shared_atlas, per_atlas_info> atlas_infos_;

  void init_bindings() {
    const auto &vert_bindings = TVertexRefl::descriptor_bindings_;

    for (auto vert_binding : vert_bindings) {
      bindings_.emplace_back(vert_binding);
    }

    for (auto frag_binding : TFragRefl::descriptor_bindings_) {
      bool in_vert_stage = false;
      for (int i = 0; i < vert_bindings.size(); i++) {
        if (frag_binding.binding == vert_bindings[i].binding) {
          bindings_[i].stageFlags |= frag_binding.stageFlags;
          in_vert_stage = true;
        }
      }
      if (!in_vert_stage)
        bindings_.emplace_back(frag_binding);
    }
  }

  void init_pools(const pipeline_params &_params) {
    std::array<VkDescriptorPoolSize, 2> descriptor_pools {
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 0},
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 0},
    };

    for (auto binding : bindings_) {
      switch (binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: descriptor_pools[0].descriptorCount += binding.descriptorCount; break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: descriptor_pools[1].descriptorCount += binding.descriptorCount; break;
        default: assert(false);
      }
    }

    if (descriptor_pools[0].descriptorCount == 0 && descriptor_pools[1].descriptorCount ==0) {
      assert(_params.max_sets_ == 0); // Force user to explicitly specify that this pipeline has no descriptor sets
      return;
    }

    for (auto &pool : descriptor_pools)
      pool.descriptorCount *= _params.max_sets_;

    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = descriptor_pools[1].descriptorCount == 0 ? 1 : 2;
    create_info.pPoolSizes = descriptor_pools.data();
    create_info.maxSets = _params.max_sets_;
    create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    if (vkCreateDescriptorPool(device_ref_, &create_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor pool!");
  }

  void init_descriptor_layout() {
    std::vector<VkDescriptorBindingFlagsEXT> bindings_flags(bindings_.size(), 0);
    for (size_t i = 0; i < bindings_.size(); i++) {
      const VkDescriptorSetLayoutBinding &binding = bindings_[i];
      if (binding.descriptorCount > 1)
        bindings_flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindings_flags_info = {};
    bindings_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    bindings_flags_info.bindingCount = bindings_flags.size();
    bindings_flags_info.pBindingFlags = bindings_flags.data();

    VkDescriptorSetLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = bindings_.size();
    create_info.pBindings = bindings_.data();
    create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    create_info.pNext = &bindings_flags_info;

    if (vkCreateDescriptorSetLayout(device_ref_, &create_info, nullptr, &descriptor_layout_) != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor set layout!");
  }

  void init_shaders() {
    VkShaderModuleCreateInfo vert_create_info = {};
    vert_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_create_info.codeSize = TVertexRefl::bytecode_.size();
    vert_create_info.pCode = (uint32_t*)TVertexRefl::bytecode_.data();

    if (HUT_PVK(vkCreateShaderModule, device_ref_, &vert_create_info, nullptr, &vert_) != VK_SUCCESS)
      throw std::runtime_error("failed to create vertex module!");

    VkShaderModuleCreateInfo frag_create_info = {};
    frag_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_create_info.codeSize = TFragRefl::bytecode_.size();
    frag_create_info.pCode = (uint32_t*)TFragRefl::bytecode_.data();

    if (HUT_PVK(vkCreateShaderModule, device_ref_, &frag_create_info, nullptr, &frag_) != VK_SUCCESS)
      throw std::runtime_error("failed to create fragment module!");
  }

  void init_pipeline_layout() {
    VkDescriptorSetLayout set_layouts[] = {descriptor_layout_};
    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pushConstantRangeCount = 0;
    layout_create_info.pPushConstantRanges = nullptr;
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = set_layouts;

    if (vkCreatePipelineLayout(device_ref_, &layout_create_info, nullptr, &layout_) != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout!");
  }

  void init_pipeline(u16vec4 _default_viewport, VkSampleCountFlagBits _samples, const pipeline_params &_params) {
    HUT_PROFILE_SCOPE(PPIPELINE, __PRETTY_FUNCTION__);
    auto size = bbox_size(_default_viewport);
    auto origin = bbox_origin(_default_viewport);

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

    std::vector<VkVertexInputBindingDescription> vertices_bindings;
    if (sizeof(vertex) >= 4)
      vertices_bindings.emplace_back(VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(vertex),   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX});
    if (sizeof(instance) >= 4)
      vertices_bindings.emplace_back(VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE});

    VkPipelineVertexInputStateCreateInfo vertex_create_info = {};
    vertex_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_create_info.vertexBindingDescriptionCount = vertices_bindings.size();
    vertex_create_info.pVertexBindingDescriptions = vertices_bindings.data();
    vertex_create_info.vertexAttributeDescriptionCount = (uint32_t)TVertexRefl::vertices_description_.size();
    vertex_create_info.pVertexAttributeDescriptions = TVertexRefl::vertices_description_.data();

    VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
    assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_create_info.topology = _params.topology_;
    assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = origin.x;
    viewport.y = size.y;
    viewport.width = size.x;
    viewport.height = size.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {origin.x, origin.y};
    scissor.extent = {size.x, size.y};

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
    rasterizer_create_info.polygonMode = _params.polygonMode_;
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
    pipelineInfo.renderPass = render_pass_;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;  // Optional

    if (HUT_PVK(vkCreateGraphicsPipelines, device_ref_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline!");
  }

public:
  explicit pipeline(render_target &_target, const pipeline_params &_params = {})
      : device_ref_(_target.display_.device_), render_pass_(_target.renderpass_) {
    HUT_PROFILE_SCOPE(PPIPELINE, __PRETTY_FUNCTION__);
    assert(render_pass_ != VK_NULL_HANDLE);

    init_bindings();
    init_pools(_params);
    init_descriptor_layout();
    if (_params.max_sets_ > 0)
      alloc_next_descriptors(1);
    init_shaders();
    init_pipeline_layout();
    init_pipeline(_target.render_target_params_.box_, _target.sample_count_, _params);
  }

  virtual ~pipeline() {
    HUT_PVK(vkDeviceWaitIdle, device_ref_);
    if (descriptor_layout_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyDescriptorSetLayout, device_ref_, descriptor_layout_, nullptr);
    if (descriptor_pool_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyDescriptorPool, device_ref_, descriptor_pool_, nullptr);
    if (pipeline_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyPipeline, device_ref_, pipeline_, nullptr);
    if (layout_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyPipelineLayout ,device_ref_, layout_, nullptr);
    if (vert_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyShaderModule, device_ref_, vert_, nullptr);
    if (frag_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyShaderModule, device_ref_, frag_, nullptr);
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

    if (vkAllocateDescriptorSets(device_ref_, &alloc_info, descriptors_.data() + current_count) != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor set!");
  }

  struct descriptor_write_context {
    std::vector<VkWriteDescriptorSet> writes_;
    std::vector<VkDescriptorImageInfo> images_;
    std::vector<VkDescriptorBufferInfo> buffers_;
    VkDescriptorSet dst_ = VK_NULL_HANDLE;
    uint descriptor_index_ = 0;

    explicit descriptor_write_context(size_t _size) {
      writes_.reserve(_size);
      images_.reserve(_size);
      buffers_.reserve(_size);
    }

    template<typename TUBO>
    void buffer(uint _binding, const shared_ref<TUBO> &_ubo) {
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

    void texture(uint _binding, const shared_image &_image, const shared_sampler &_sampler) {
      VkDescriptorImageInfo info = {};
      info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      info.imageView = _image->view_;
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

    void atlas(uint _binding, const shared_atlas &_atlas, const shared_sampler &_sampler) {
      for (auto &page : _atlas->pages_) {
        VkDescriptorImageInfo info = {};
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = page.image_->view_;
        info.sampler = _sampler->sampler_;
        images_.emplace_back(info);
      }

      VkWriteDescriptorSet write = {};
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = dst_;
      write.dstBinding = _binding;
      write.dstArrayElement = 0;
      write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount = _atlas->page_count();
      write.pImageInfo = &images_.back() - (_atlas->page_count() - 1);
      writes_.emplace_back(write);
    }
  };

  void write_continue(int _binding, descriptor_write_context &_context) {}

  template<typename TBufferType, typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_ref<TBufferType> &_buffer, const TRest&... _rest) {
    _context.buffer(_binding, _buffer);
    write_continue(_binding + 1, _context, std::forward<const TRest&>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_image &_image, const shared_sampler &_sampler, const TRest&... _rest) {
    _context.texture(_binding, _image, _sampler);
    write_continue(_binding + 1, _context, std::forward<const TRest&>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_image &_image0, const shared_image &_image1, const shared_sampler &_sampler, const TRest&... _rest) {
    _context.texture(_binding, _image0, _sampler);
    _context.texture(_binding + 1, _image1, _sampler);
    write_continue(_binding + 2, _context, std::forward<const TRest&>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_image &_image0, const shared_image &_image1, const shared_image &_image2, const shared_sampler &_sampler, const TRest&... _rest) {
    _context.texture(_binding, _image0, _sampler);
    _context.texture(_binding + 1, _image1, _sampler);
    _context.texture(_binding + 2, _image2, _sampler);
    write_continue(_binding + 3, _context, std::forward<const TRest&>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_image &_image0, const shared_image &_image1, const shared_image &_image2, const shared_image &_image3, const shared_sampler &_sampler, const TRest&... _rest) {
    _context.texture(_binding, _image0, _sampler);
    _context.texture(_binding + 1, _image1, _sampler);
    _context.texture(_binding + 2, _image2, _sampler);
    _context.texture(_binding + 3, _image3, _sampler);
    write_continue(_binding + 4, _context, std::forward<const TRest&>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_atlas &_atlas, const shared_sampler &_sampler, const TRest&... _rest) {
    _context.atlas(_binding, _atlas, _sampler);
    write_continue(_binding + 1, _context, std::forward<const TRest&>(_rest)...);

    auto &atlas_info = atlas_infos_[_atlas];
    atlas_info.binding_ = _binding;
    auto &desc_info = atlas_info.descriptors_info_[_context.descriptor_index_];
    desc_info.sampler_ = _sampler;
    desc_info.last_bound_ = _atlas->page_count();
  }

  void write(uint _descriptor_index, const TExtraAttachments&... _attachments) {
    assert(_descriptor_index < descriptors_.size());
    attachments_.emplace(_descriptor_index, attachments{std::forward_as_tuple(_attachments...)});

    descriptor_write_context filler{sizeof...(_attachments)};
    filler.descriptor_index_ = _descriptor_index;
    filler.dst_ = descriptors_[_descriptor_index];
    write_continue(0, filler, std::forward<const TExtraAttachments&>(_attachments)...);

    HUT_PVK(vkUpdateDescriptorSets, device_ref_, filler.writes_.size(), filler.writes_.data(), 0, nullptr);
  }

  void update_atlas(uint _descriptor_index, const shared_atlas &_atlas) {
    assert(descriptor_attached(_descriptor_index));
    assert(atlas_infos_.find(_atlas) != atlas_infos_.end());
    auto &atlas_status = atlas_infos_[_atlas];
    assert(atlas_status.descriptors_info_.find(_descriptor_index) != atlas_status.descriptors_info_.end());
    auto &desc_info = atlas_status.descriptors_info_[_descriptor_index];
    uint new_count = _atlas->page_count();

    assert(desc_info.last_bound_ <= new_count);
    uint diff = new_count - desc_info.last_bound_;
    if (diff > 0) {
      std::vector<VkDescriptorImageInfo> image_infos(diff);
      for (uint i = 0; i < diff; i++) {
        VkDescriptorImageInfo &info = image_infos[i];
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = _atlas->pages_[desc_info.last_bound_ + i].image_->view_;
        info.sampler = desc_info.sampler_->sampler_;
      }

      VkWriteDescriptorSet write = {};
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = descriptors_[_descriptor_index];
      write.dstBinding = atlas_status.binding_;
      write.dstArrayElement = desc_info.last_bound_;
      write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount = image_infos.size();
      write.pImageInfo = image_infos.data();

      HUT_PVK(vkUpdateDescriptorSets, device_ref_, 1, &write, 0, nullptr);
      desc_info.last_bound_ = new_count;
    }
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

  void draw(VkCommandBuffer _buffer, uint _vertex_count, uint _instances_count, uint _vertex_offset, uint _instances_offset)  {
    HUT_PVK(vkCmdDraw, _buffer, _vertex_count, _instances_count, _vertex_offset, _instances_offset);
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

struct proj_ubo {
  mat4 proj_ {1};
};

using rgb        = pipeline<uint16_t, hut_shaders::rgb_vert_spv_refl, hut_shaders::rgb_frag_spv_refl, const shared_ref<proj_ubo> &>;
using rgba       = pipeline<uint16_t, hut_shaders::rgba_vert_spv_refl, hut_shaders::rgba_frag_spv_refl, const shared_ref<proj_ubo> &>;
using tex        = pipeline<uint16_t, hut_shaders::tex_vert_spv_refl, hut_shaders::tex_frag_spv_refl, const shared_ref<proj_ubo> &, const shared_image &, const shared_sampler &>;
using atlas      = pipeline<uint16_t, hut_shaders::tex_vert_spv_refl, hut_shaders::atlas_frag_spv_refl, const shared_ref<proj_ubo> &, const shared_atlas &, const shared_sampler &>;
using tex_rgb    = pipeline<uint16_t, hut_shaders::tex_rgb_vert_spv_refl, hut_shaders::tex_rgb_frag_spv_refl, const shared_ref<proj_ubo> &, const shared_image &, const shared_sampler &>;
using tex_rgba   = pipeline<uint16_t, hut_shaders::tex_rgba_vert_spv_refl, hut_shaders::tex_rgba_frag_spv_refl, const shared_ref<proj_ubo> &, const shared_image &, const shared_sampler &>;
using tex_mask   = pipeline<uint16_t, hut_shaders::tex_mask_vert_spv_refl, hut_shaders::tex_mask_frag_spv_refl, const shared_ref<proj_ubo> &, const shared_image &, const shared_sampler &>;
using atlas_mask = pipeline<uint16_t, hut_shaders::tex_mask_vert_spv_refl, hut_shaders::atlas_mask_frag_spv_refl, const shared_ref<proj_ubo> &, const shared_atlas &, const shared_sampler &>;
using box_rgba   = pipeline<uint16_t, hut_shaders::box_rgba_vert_spv_refl, hut_shaders::box_rgba_frag_spv_refl, const shared_ref<proj_ubo> &>;

}  // namespace hut
