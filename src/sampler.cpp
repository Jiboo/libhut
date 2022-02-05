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

#include "hut/sampler.hpp"

#include <cstring>

#include <iostream>

#include "hut/utils/format.hpp"
#include "hut/utils/profiling.hpp"

#include "hut/display.hpp"

using namespace hut;

sampler::sampler(display &_display, const sampler_params &_params)
    : sampler_(VK_NULL_HANDLE)
    , device_(_display.device()) {
  HUT_PROFILE_FUN(PSAMPLER)
  VkSamplerCreateInfo sampler_info     = {};
  sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter               = _params.filter_;
  sampler_info.minFilter               = _params.filter_;
  sampler_info.addressModeU            = _params.addressMode_;
  sampler_info.addressModeV            = _params.addressMode_;
  sampler_info.addressModeW            = _params.addressMode_;
  bool enable_filtering                = _params.anisotropy_ && _display.features().samplerAnisotropy;
  sampler_info.anisotropyEnable        = enable_filtering ? VK_TRUE : VK_FALSE;
  sampler_info.maxAnisotropy           = enable_filtering ? _display.properties().limits.maxSamplerAnisotropy : 0;
  sampler_info.borderColor             = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable           = VK_FALSE;
  sampler_info.compareOp               = VK_COMPARE_OP_NEVER;
  sampler_info.mipmapMode
      = _params.filter_ == VK_FILTER_NEAREST ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = _params.lodBias_;
  sampler_info.minLod     = _params.lodRange_.x;
  sampler_info.maxLod     = _params.lodRange_.y;
  if (vkCreateSampler(device_, &sampler_info, nullptr, &sampler_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

sampler::~sampler() {
  if (sampler_) {
    HUT_PROFILE_FUN(PSAMPLER)
    HUT_PVK(vkDeviceWaitIdle, device_);
    HUT_PVK(vkDestroySampler, device_, sampler_, nullptr);
  }
}
