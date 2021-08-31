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

#include "hut/utils/math.hpp"

namespace hut {

struct sampler_params {
  VkFilter             filter_      = VK_FILTER_LINEAR;
  bool                 anisotropy_  = true;
  VkSamplerAddressMode addressMode_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  vec2                 lodRange_    = {0, 0};
  float                lodBias_     = 0;

  sampler_params &fast() {
    filter_     = VK_FILTER_NEAREST;
    anisotropy_ = false;
    return *this;
  }

  sampler_params &lod(float _max, float _min = 0, float _bias = 0) {
    lodRange_.x = _min;
    lodRange_.y = _max;
    lodBias_    = _bias;
    return *this;
  }
};

class sampler {
  VkSampler sampler_;
  VkDevice  device_;

 public:
  explicit sampler(display &_display, const sampler_params &_params = {});
  ~sampler();

  VkSampler underlying() { return sampler_; }
};

}  // namespace hut
