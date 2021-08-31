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

#include <cstring>

#include <iostream>

#include "hut/utils/format.hpp"
#include "hut/utils/profiling.hpp"

#include "hut/display.hpp"
#include "hut/sampler.hpp"

using namespace hut;

sampler::sampler(display &_display, const sampler_params &_params)
    : device_(_display.device())
    , sampler_(VK_NULL_HANDLE) {
  VkSamplerCreateInfo samplerInfo     = {};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = _params.filter_;
  samplerInfo.minFilter               = _params.filter_;
  samplerInfo.addressModeU            = _params.addressMode_;
  samplerInfo.addressModeV            = _params.addressMode_;
  samplerInfo.addressModeW            = _params.addressMode_;
  bool enable_filtering               = _params.anisotropy_ && _display.features().samplerAnisotropy;
  samplerInfo.anisotropyEnable        = enable_filtering ? VK_TRUE : VK_FALSE;
  samplerInfo.maxAnisotropy           = enable_filtering ? _display.properties().limits.maxSamplerAnisotropy : 0;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_NEVER;
  samplerInfo.mipmapMode              = _params.filter_ == VK_FILTER_NEAREST ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = _params.lodBias_;
  samplerInfo.minLod                  = _params.lodRange_.x;
  samplerInfo.maxLod                  = _params.lodRange_.y;
  if (vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

sampler::~sampler() {
  HUT_PVK(vkDeviceWaitIdle, device_);
  HUT_PVK(vkDestroySampler, device_, sampler_, nullptr);
}
