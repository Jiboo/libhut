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

#include <vulkan/vulkan.h>

#include "hut/utils/fwd.hpp"
#include "hut/utils/math.hpp"
#include "hut/utils/profiling.hpp"

#include "hut/atlas.hpp"
#include "hut/buffer.hpp"
#include "hut/display.hpp"
#include "hut/image.hpp"
#include "hut/sampler.hpp"
#include "hut/suballoc.hpp"
#include "hut/window.hpp"

namespace hut {

// This UBO is used by most shaders in hut provided renderers
struct common_ubo {
  mat4  proj_{1};
  mat4  view_{1};
  float dpi_scale_ = 1;

  explicit common_ubo(u16vec2 _size) { proj_ = ortho<float>(0.f, float(_size.x), 0.f, float(_size.y)); }
};

using shared_ubo = shared_buffer_suballoc<common_ubo>;

using shared_commands         = shared_buffer_suballoc<VkDrawIndirectCommand>;
using shared_indexed_commands = shared_buffer_suballoc<VkDrawIndexedIndirectCommand>;

struct pipeline_params {
  VkPrimitiveTopology topology_        = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkPolygonMode       polygon_mode_    = VK_POLYGON_MODE_FILL;
  VkCompareOp         depth_compare_   = VK_COMPARE_OP_LESS_OR_EQUAL;
  VkCullModeFlagBits  cull_mode_       = VK_CULL_MODE_NONE;
  VkFrontFace         front_face_      = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  VkBool32            enable_blending_ = VK_TRUE;
  u32                 max_sets_ = 16, initial_sets_ = 1;
};

namespace details {
template<typename TIndex>
struct vulkan_index_type {
  constexpr static VkIndexType TYPE = VK_INDEX_TYPE_MAX_ENUM;
};
template<>
struct vulkan_index_type<u16> {
  constexpr static VkIndexType TYPE = VK_INDEX_TYPE_UINT16;
};
template<>
struct vulkan_index_type<u32> {
  constexpr static VkIndexType TYPE = VK_INDEX_TYPE_UINT32;
};
}  // namespace details

template<typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments>
class pipeline {
 public:
  using index_t          = TIndice;
  using vertex           = typename TVertexRefl::vertex;
  using instance         = typename TVertexRefl::instance;
  using shared_indices   = shared_buffer_suballoc<index_t>;
  using shared_vertices  = shared_buffer_suballoc<vertex>;
  using shared_instances = shared_buffer_suballoc<instance>;

 private:
  using extra_attachments = std::tuple<TExtraAttachments...>;

  struct attachments {
    extra_attachments extras_;
  };

  VkDevice     device_ref_;
  VkRenderPass render_pass_;

  VkShaderModule        vert_              = VK_NULL_HANDLE;
  VkShaderModule        frag_              = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout      layout_            = VK_NULL_HANDLE;
  VkPipeline            pipeline_          = VK_NULL_HANDLE;
  VkDescriptorPool      descriptor_pool_   = VK_NULL_HANDLE;

  std::vector<VkDescriptorSetLayoutBinding> bindings_;
  std::vector<VkDescriptorSet>              descriptors_;
  std::unordered_map<uint, attachments>     attachments_;

  struct per_descriptor_atlas_info {
    shared_sampler sampler_;
    uint           last_bound_ = 0;
  };
  struct per_atlas_info {
    std::unordered_map<uint, per_descriptor_atlas_info> descriptors_info_;
    uint                                                binding_;
  };
  std::unordered_map<shared_atlas, per_atlas_info> atlas_infos_;

  void init_bindings() {
    constexpr auto &VBINDINGS = TVertexRefl::DESCRIPTOR_BINDINGS;

    for (auto &VBINDING : VBINDINGS) {
      bindings_.emplace_back(VBINDING);
    }

    for (auto &FBINDING : TFragRefl::DESCRIPTOR_BINDINGS) {
      bool in_vert_stage = false;
      for (u32 i = 0; i < VBINDINGS.size(); i++) {
        if (FBINDING.binding == VBINDINGS[i].binding) {
          bindings_[i].stageFlags |= FBINDING.stageFlags;
          in_vert_stage = true;
        }
      }
      if (!in_vert_stage)
        bindings_.emplace_back(FBINDING);
    }
  }

  void init_pools(const pipeline_params &_params) {
    std::array<VkDescriptorPoolSize, 2> descriptor_pools{
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 0},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 0},
    };

    for (auto binding : bindings_) {
      switch (binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: descriptor_pools[0].descriptorCount += binding.descriptorCount; break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
          descriptor_pools[1].descriptorCount += binding.descriptorCount;
          break;
        default: assert(false);
      }
    }

    if (descriptor_pools[0].descriptorCount == 0 && descriptor_pools[1].descriptorCount == 0) {
      assert(_params.max_sets_ == 0);  // Force user to explicitly specify that this pipeline has no descriptor sets
      return;
    }

    for (auto &pool : descriptor_pools)
      pool.descriptorCount *= _params.max_sets_;

    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount              = descriptor_pools[1].descriptorCount == 0 ? 1 : 2;
    create_info.pPoolSizes                 = descriptor_pools.data();
    create_info.maxSets                    = _params.max_sets_;
    create_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

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
    bindings_flags_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    bindings_flags_info.bindingCount  = bindings_flags.size();
    bindings_flags_info.pBindingFlags = bindings_flags.data();

    VkDescriptorSetLayoutCreateInfo create_info = {};
    create_info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount                    = bindings_.size();
    create_info.pBindings                       = bindings_.data();
    create_info.flags                           = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    create_info.pNext                           = &bindings_flags_info;

    if (vkCreateDescriptorSetLayout(device_ref_, &create_info, nullptr, &descriptor_layout_) != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor set layout!");
  }

  void init_shaders() {
    VkShaderModuleCreateInfo vert_create_info = {};
    vert_create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_create_info.codeSize                 = TVertexRefl::BYTECODE.size();
    vert_create_info.pCode                    = (u32 *)TVertexRefl::BYTECODE.data();

    if (HUT_PVK(vkCreateShaderModule, device_ref_, &vert_create_info, nullptr, &vert_) != VK_SUCCESS)
      throw std::runtime_error("failed to create vertex module!");

    VkShaderModuleCreateInfo frag_create_info = {};
    frag_create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_create_info.codeSize                 = TFragRefl::BYTECODE.size();
    frag_create_info.pCode                    = (u32 *)TFragRefl::BYTECODE.data();

    if (HUT_PVK(vkCreateShaderModule, device_ref_, &frag_create_info, nullptr, &frag_) != VK_SUCCESS)
      throw std::runtime_error("failed to create fragment module!");
  }

  void init_pipeline_layout() {
    VkDescriptorSetLayout      set_layouts[]      = {descriptor_layout_};
    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pushConstantRangeCount     = 0;
    layout_create_info.pPushConstantRanges        = nullptr;
    layout_create_info.setLayoutCount             = 1;
    layout_create_info.pSetLayouts                = set_layouts;

    if (vkCreatePipelineLayout(device_ref_, &layout_create_info, nullptr, &layout_) != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout!");
  }

  void init_pipeline(u16vec4 _default_viewport, VkSampleCountFlagBits _samples, const pipeline_params &_params) {
    HUT_PROFILE_SCOPE(PPIPELINE, "pipeline({},{})::init_pipeline", TVertexRefl::FILENAME, TFragRefl::FILENAME)
    auto size   = bbox_size(_default_viewport);
    auto origin = bbox_origin(_default_viewport);

    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module                          = vert_;
    vert_stage_info.pName                           = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module                          = frag_;
    frag_stage_info.pName                           = "main";

    VkPipelineShaderStageCreateInfo shader_create_infos[] = {vert_stage_info, frag_stage_info};

    std::vector<VkVertexInputBindingDescription> vertices_bindings;
    if (sizeof(vertex) >= 4)
      vertices_bindings.emplace_back(VkVertexInputBindingDescription{
          .binding = 0, .stride = sizeof(vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX});
    if (sizeof(instance) >= 4)
      vertices_bindings.emplace_back(VkVertexInputBindingDescription{
          .binding = 1, .stride = sizeof(instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE});

    VkPipelineVertexInputStateCreateInfo vertex_create_info = {};
    vertex_create_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_create_info.vertexBindingDescriptionCount        = vertices_bindings.size();
    vertex_create_info.pVertexBindingDescriptions           = vertices_bindings.data();
    vertex_create_info.vertexAttributeDescriptionCount      = (u32)TVertexRefl::VERTICES_DESCRIPTION.size();
    vertex_create_info.pVertexAttributeDescriptions         = TVertexRefl::VERTICES_DESCRIPTION.data();

    VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
    assembly_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_create_info.topology               = _params.topology_;
    assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x          = origin.x;
    viewport.y          = size.y;
    viewport.width      = size.x;
    viewport.height     = size.y;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor = {};
    scissor.offset   = {origin.x, origin.y};
    scissor.extent   = {size.x, size.y};

    VkPipelineViewportStateCreateInfo viewport_create_info = {};
    viewport_create_info.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.viewportCount                     = 1;
    viewport_create_info.pViewports                        = &viewport;
    viewport_create_info.scissorCount                      = 1;
    viewport_create_info.pScissors                         = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {};
    rasterizer_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_create_info.depthClampEnable        = VK_FALSE;
    rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_create_info.polygonMode             = _params.polygon_mode_;
    rasterizer_create_info.lineWidth               = 1.0f;
    rasterizer_create_info.cullMode                = _params.cull_mode_;
    rasterizer_create_info.frontFace               = _params.front_face_;
    rasterizer_create_info.depthBiasEnable         = VK_FALSE;
    rasterizer_create_info.depthBiasConstantFactor = 0.0f;
    rasterizer_create_info.depthBiasClamp          = 0.0f;
    rasterizer_create_info.depthBiasSlopeFactor    = 0.0f;

    VkPipelineMultisampleStateCreateInfo msaa_create_info = {};
    msaa_create_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa_create_info.sampleShadingEnable                  = VK_FALSE;
    msaa_create_info.rasterizationSamples                 = _samples;
    msaa_create_info.minSampleShading                     = 1.0f;
    msaa_create_info.pSampleMask                          = nullptr;
    msaa_create_info.alphaToCoverageEnable                = VK_FALSE;
    msaa_create_info.alphaToOneEnable                     = VK_FALSE;

    constexpr uint COMPONENT_MASK
        = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendAttachmentState blend_attachment = {};
    blend_attachment.colorWriteMask                      = COMPONENT_MASK;
    blend_attachment.blendEnable                         = _params.enable_blending_;
    blend_attachment.srcColorBlendFactor                 = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstColorBlendFactor                 = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp                        = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.alphaBlendOp                        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blend_create_info = {};
    blend_create_info.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_create_info.logicOpEnable                       = VK_FALSE;
    blend_create_info.logicOp                             = VK_LOGIC_OP_COPY;
    blend_create_info.attachmentCount                     = 1;
    blend_create_info.pAttachments                        = &blend_attachment;
    blend_create_info.blendConstants[0]                   = 0.0f;
    blend_create_info.blendConstants[1]                   = 0.0f;
    blend_create_info.blendConstants[2]                   = 0.0f;
    blend_create_info.blendConstants[3]                   = 0.0f;

    const std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_states_create_info = {};
    dynamic_states_create_info.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_states_create_info.dynamicStateCount                = (u32)dynamic_states.size();
    dynamic_states_create_info.pDynamicStates                   = dynamic_states.data();

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable                       = VK_TRUE;
    depth_stencil.depthWriteEnable                      = VK_TRUE;
    depth_stencil.depthCompareOp                        = _params.depth_compare_;
    depth_stencil.depthBoundsTestEnable                 = VK_FALSE;
    depth_stencil.minDepthBounds                        = 0.0f;  // Optional
    depth_stencil.maxDepthBounds                        = 1.0f;  // Optional
    depth_stencil.stencilTestEnable                     = VK_FALSE;
    depth_stencil.front                                 = {};  // Optional
    depth_stencil.back                                  = {};  // Optional

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount                   = 2;
    pipeline_info.pStages                      = shader_create_infos;
    pipeline_info.pVertexInputState            = &vertex_create_info;
    pipeline_info.pInputAssemblyState          = &assembly_create_info;
    pipeline_info.pViewportState               = &viewport_create_info;
    pipeline_info.pRasterizationState          = &rasterizer_create_info;
    pipeline_info.pMultisampleState            = &msaa_create_info;
    pipeline_info.pColorBlendState             = &blend_create_info;
    pipeline_info.pDynamicState                = &dynamic_states_create_info;
    pipeline_info.pDepthStencilState           = &depth_stencil;
    pipeline_info.layout                       = layout_;
    pipeline_info.renderPass                   = render_pass_;
    pipeline_info.subpass                      = 0;
    pipeline_info.basePipelineHandle           = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex            = -1;  // Optional

    if (HUT_PVK(vkCreateGraphicsPipelines, device_ref_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_)
        != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline!");
  }

 public:
  pipeline() = delete;

  pipeline(const pipeline &) = delete;
  pipeline &operator=(const pipeline &) = delete;

  pipeline(pipeline &&) noexcept = delete;
  pipeline &operator=(pipeline &&) noexcept = delete;

  explicit pipeline(render_target &_target, const pipeline_params &_params = {})
      : device_ref_(_target.parent().device())
      , render_pass_(_target.renderpass()) {
    HUT_PROFILE_SCOPE(PPIPELINE, "pipeline({},{})::pipeline", TVertexRefl::FILENAME, TFragRefl::FILENAME)
    assert(render_pass_ != VK_NULL_HANDLE);

    init_bindings();
    init_pools(_params);
    init_descriptor_layout();
    if (_params.max_sets_ > 0)
      resize_descriptors(_params.initial_sets_);
    init_shaders();
    init_pipeline_layout();
    init_pipeline(_target.params().box_, _target.sample_count(), _params);
  }

  ~pipeline() {
    HUT_PVK(vkDeviceWaitIdle, device_ref_);
    if (descriptor_layout_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyDescriptorSetLayout, device_ref_, descriptor_layout_, nullptr);
    if (descriptor_pool_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyDescriptorPool, device_ref_, descriptor_pool_, nullptr);
    if (pipeline_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyPipeline, device_ref_, pipeline_, nullptr);
    if (layout_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyPipelineLayout, device_ref_, layout_, nullptr);
    if (vert_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyShaderModule, device_ref_, vert_, nullptr);
    if (frag_ != VK_NULL_HANDLE)
      HUT_PVK(vkDestroyShaderModule, device_ref_, frag_, nullptr);
  }

  void resize_descriptors(uint _count) {
    uint current_count = descriptors_.size();
    if (_count <= current_count)
      return;
    uint alloc_needed = _count - current_count;
    descriptors_.resize(_count);

    VkDescriptorSetLayout layouts[alloc_needed];
    for (uint i = 0; i < alloc_needed; i++)
      layouts[i] = descriptor_layout_;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool              = descriptor_pool_;
    alloc_info.descriptorSetCount          = alloc_needed;
    alloc_info.pSetLayouts                 = layouts;

    if (vkAllocateDescriptorSets(device_ref_, &alloc_info, descriptors_.data() + current_count) != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor set!");
  }

  uint count_descriptors() { return descriptors_.size(); }

  struct descriptor_write_context {
    std::vector<VkWriteDescriptorSet>   writes_;
    std::vector<VkDescriptorImageInfo>  images_;
    std::vector<VkDescriptorBufferInfo> buffers_;
    VkDescriptorSet                     dst_              = VK_NULL_HANDLE;
    uint                                descriptor_index_ = 0;

    explicit descriptor_write_context(size_t _size) {
      writes_.reserve(_size);
      images_.reserve(_size);
      buffers_.reserve(_size);
    }

    template<typename TUBO>
    void buffer(uint _binding, const shared_buffer_suballoc<TUBO> &_ubo) {
      VkDescriptorBufferInfo info = {};
      info.buffer                 = _ubo->parent()->buffer_;
      info.offset                 = _ubo->offset_bytes();
      info.range                  = _ubo->size_bytes();
      buffers_.emplace_back(info);

      VkWriteDescriptorSet write = {};
      write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet               = dst_;
      write.dstBinding           = _binding;
      write.dstArrayElement      = 0;
      write.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write.descriptorCount      = 1;
      write.pBufferInfo          = &buffers_.back();
      writes_.emplace_back(write);
    }

    void texture(uint _binding, const shared_image &_image, const shared_sampler &_sampler) {
      VkDescriptorImageInfo info = {};
      info.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      info.imageView             = _image->view();
      info.sampler               = _sampler->underlying();
      images_.emplace_back(info);

      VkWriteDescriptorSet write = {};
      write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet               = dst_;
      write.dstBinding           = _binding;
      write.dstArrayElement      = 0;
      write.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount      = 1;
      write.pImageInfo           = &images_.back();
      writes_.emplace_back(write);
    }

    void atlas(uint _binding, const shared_atlas &_atlas, const shared_sampler &_sampler) {
      for (auto &page : _atlas->pages_) {
        VkDescriptorImageInfo info = {};
        info.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView             = page.image_->view();
        info.sampler               = _sampler->underlying();
        images_.emplace_back(info);
      }

      VkWriteDescriptorSet write = {};
      write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet               = dst_;
      write.dstBinding           = _binding;
      write.dstArrayElement      = 0;
      write.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount      = _atlas->page_count();
      write.pImageInfo           = &images_.back() - (_atlas->page_count() - 1);
      writes_.emplace_back(write);
    }
  };

  void write_continue(int _binding, descriptor_write_context &_context) {}

  template<typename TBufferType, typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context,
                      const shared_buffer_suballoc<TBufferType> &_buffer, const TRest &..._rest) {
    _context.buffer(_binding, _buffer);
    write_continue(_binding + 1, _context, std::forward<const TRest &>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_image &_image,
                      const shared_sampler &_sampler, const TRest &..._rest) {
    _context.texture(_binding, _image, _sampler);
    write_continue(_binding + 1, _context, std::forward<const TRest &>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_image &_image0,
                      const shared_image &_image1, const shared_sampler &_sampler, const TRest &..._rest) {
    _context.texture(_binding, _image0, _sampler);
    _context.texture(_binding + 1, _image1, _sampler);
    write_continue(_binding + 2, _context, std::forward<const TRest &>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_image &_image0,
                      const shared_image &_image1, const shared_image &_image2, const shared_sampler &_sampler,
                      const TRest &..._rest) {
    _context.texture(_binding, _image0, _sampler);
    _context.texture(_binding + 1, _image1, _sampler);
    _context.texture(_binding + 2, _image2, _sampler);
    write_continue(_binding + 3, _context, std::forward<const TRest &>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_image &_image0,
                      const shared_image &_image1, const shared_image &_image2, const shared_image &_image3,
                      const shared_sampler &_sampler, const TRest &..._rest) {
    _context.texture(_binding, _image0, _sampler);
    _context.texture(_binding + 1, _image1, _sampler);
    _context.texture(_binding + 2, _image2, _sampler);
    _context.texture(_binding + 3, _image3, _sampler);
    write_continue(_binding + 4, _context, std::forward<const TRest &>(_rest)...);
  }

  template<typename... TRest>
  void write_continue(int _binding, descriptor_write_context &_context, const shared_atlas &_atlas,
                      const shared_sampler &_sampler, const TRest &..._rest) {
    _context.atlas(_binding, _atlas, _sampler);
    write_continue(_binding + 1, _context, std::forward<const TRest &>(_rest)...);

    auto &atlas_info      = atlas_infos_[_atlas];
    atlas_info.binding_   = _binding;
    auto &desc_info       = atlas_info.descriptors_info_[_context.descriptor_index_];
    desc_info.sampler_    = _sampler;
    desc_info.last_bound_ = _atlas->page_count();
  }

  void write(uint _descriptor_index, const TExtraAttachments &..._attachments) {
    assert(_descriptor_index < descriptors_.size());
    attachments_.emplace(_descriptor_index, attachments{std::forward_as_tuple(_attachments...)});

    descriptor_write_context filler{sizeof...(_attachments)};
    filler.descriptor_index_ = _descriptor_index;
    filler.dst_              = descriptors_[_descriptor_index];
    write_continue(0, filler, std::forward<const TExtraAttachments &>(_attachments)...);

    HUT_PVK(vkUpdateDescriptorSets, device_ref_, filler.writes_.size(), filler.writes_.data(), 0, nullptr);
  }

  void update_atlas(uint _descriptor_index, const shared_atlas &_atlas) {
    assert(descriptor_attached(_descriptor_index));
    assert(atlas_infos_.find(_atlas) != atlas_infos_.end());
    auto &atlas_status = atlas_infos_[_atlas];
    assert(atlas_status.descriptors_info_.find(_descriptor_index) != atlas_status.descriptors_info_.end());
    auto &desc_info = atlas_status.descriptors_info_[_descriptor_index];
    uint  new_count = _atlas->page_count();

    assert(desc_info.last_bound_ <= new_count);
    uint diff = new_count - desc_info.last_bound_;
    if (diff > 0) {
      std::vector<VkDescriptorImageInfo> image_infos(diff);
      for (uint i = 0; i < diff; i++) {
        VkDescriptorImageInfo &info = image_infos[i];
        info.imageLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView              = _atlas->pages_[desc_info.last_bound_ + i].image_->view();
        info.sampler                = desc_info.sampler_->underlying();
      }

      VkWriteDescriptorSet write = {};
      write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet               = descriptors_[_descriptor_index];
      write.dstBinding           = atlas_status.binding_;
      write.dstArrayElement      = desc_info.last_bound_;
      write.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount      = image_infos.size();
      write.pImageInfo           = image_infos.data();

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
    HUT_PVK(vkCmdBindDescriptorSets, _buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, 0, 1,
            descriptors_.data() + _descriptor_index, 0, nullptr);
  }

  void bind_vertices(VkCommandBuffer _buffer, const shared_vertices &_vertices) {
    assert(_vertices);
    VkBuffer     vertices_buffers[] = {_vertices->parent()->buffer_};
    VkDeviceSize vertices_offsets[] = {_vertices->offset_bytes()};
    HUT_PVK(vkCmdBindVertexBuffers, _buffer, 0, 1, vertices_buffers, vertices_offsets);
  }

  void bind_instances(VkCommandBuffer _buffer, const shared_instances &_instances) {
    if constexpr (sizeof(instance) > 1) {
      assert(_instances);
      VkBuffer     instances_buffers[] = {_instances->parent()->buffer_};
      VkDeviceSize instances_offsets[] = {_instances->offset_bytes()};
      HUT_PVK(vkCmdBindVertexBuffers, _buffer, 1, 1, instances_buffers, instances_offsets);
    }
  }

  void bind_indices(VkCommandBuffer _buffer, const shared_indices &_indices) {
    assert(_indices);
    constexpr auto VULKAN_INDEX_TYPE = details::vulkan_index_type<index_t>::TYPE;
    static_assert(VULKAN_INDEX_TYPE != VK_INDEX_TYPE_MAX_ENUM, "Unsupported index type");
    HUT_PVK(vkCmdBindIndexBuffer, _buffer, _indices->parent()->buffer_, _indices->offset_bytes(), VULKAN_INDEX_TYPE);
  }

  void draw(VkCommandBuffer _buffer, uint _vertex_count, uint _instances_count, uint _vertex_offset,
            uint _instances_offset) {
    HUT_PVK(vkCmdDraw, _buffer, _vertex_count, _instances_count, _vertex_offset, _instances_offset);
  }

  void draw_indexed(VkCommandBuffer _buffer, uint _indices_count, uint _instances_count, uint _indices_offset,
                    uint _vertex_offset, uint _instances_offset) {
    HUT_PVK(vkCmdDrawIndexed, _buffer, _indices_count, _instances_count, _indices_offset, _vertex_offset,
            _instances_offset);
  }

  void draw(VkCommandBuffer _buffer, const shared_commands &_commands, uint _commands_count, uint _stride_bytes) {
    HUT_PVK(vkCmdDrawIndirect, _buffer, _commands->parent()->buffer_, _commands->offset_bytes(), _commands_count,
            _stride_bytes);
  }

  void draw_indexed(VkCommandBuffer _buffer, const shared_indexed_commands &_commands, uint _commands_count,
                    uint _stride_bytes) {
    HUT_PVK(vkCmdDrawIndexedIndirect, _buffer, _commands->parent()->buffer_, _commands->offset_bytes(), _commands_count,
            _stride_bytes);
  }

  void draw(VkCommandBuffer _buffer, uint _descriptor_index, const shared_indices &_indices, uint _indices_offset,
            uint _indices_count, const shared_instances &_instances, uint _instances_offset, uint _instances_count,
            const shared_vertices &_vertices, uint _vertex_offset) {
    HUT_PROFILE_SCOPE(PPIPELINE, "pipeline({},{})::draw", TVertexRefl::FILENAME, TFragRefl::FILENAME)

    bind_pipeline(_buffer);
    bind_descriptor(_buffer, _descriptor_index);
    bind_vertices(_buffer, _vertices);
    bind_instances(_buffer, _instances);
    bind_indices(_buffer, _indices);
    draw_indexed(_buffer, _indices_count, _instances_count, _indices_offset, _vertex_offset, _instances_offset);
  }

  void draw(VkCommandBuffer _buffer, uint _descriptor_index, const shared_indices &_indices,
            const shared_instances &_instances, const shared_vertices &_vertices) {
    draw(_buffer, _descriptor_index, _indices, 0, _indices->size(), _instances, 0, _instances ? _instances->size() : 1,
         _vertices, 0);
  }

  void draw(VkCommandBuffer _buffer, uint _descriptor_index, const shared_indices &_indices, uint _indices_count,
            const shared_instances &_instances, const shared_vertices &_vertices) {
    draw(_buffer, _descriptor_index, _indices, 0, _indices_count, _instances, 0, _instances ? _instances->size() : 1,
         _vertices, 0);
  }
};

}  // namespace hut
