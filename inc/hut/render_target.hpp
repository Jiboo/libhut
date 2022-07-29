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

#include <string>

#include "hut/utils/chrono.hpp"
#include "hut/utils/bbox.hpp"

#include "hut/display.hpp"

namespace hut {
struct render_target_params {
  enum flag {
    FDEPTH,
    FMULTISAMPLING,
    FLAG_LAST_VALUE = FMULTISAMPLING,
  };
  using flags = flagged<flag, flag::FLAG_LAST_VALUE>;

  u16bbox_px               box_;
  VkFormat                 format_;
  VkClearColorValue        clear_color_         = {{0, 0, 0, 1}};
  VkClearDepthStencilValue clear_depth_stencil_ = {1, 0};
  VkImageLayout            initial_layout_      = VK_IMAGE_LAYOUT_UNDEFINED;
  VkImageLayout            final_layout_        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  flags                    flags_{};
};

class render_target {
 public:
  render_target() = delete;

  render_target(const render_target &)            = delete;
  render_target &operator=(const render_target &) = delete;

  render_target(render_target &&) noexcept            = delete;
  render_target &operator=(render_target &&) noexcept = delete;

  explicit render_target(display &_display);
  virtual ~render_target();

  [[nodiscard]] const render_target_params &params() const { return render_target_params_; }
  [[nodiscard]] display                    &parent() { return *display_; }

  [[nodiscard]] VkSampleCountFlagBits sample_count() const { return sample_count_; }
  [[nodiscard]] VkRenderPass          renderpass() const { return renderpass_; }

 protected:
  display             *display_;
  render_target_params render_target_params_;

  shared_image          depth_;
  shared_image          msaa_rendertarget_;
  VkSampleCountFlagBits sample_count_ = VK_SAMPLE_COUNT_1_BIT;

  VkRenderPass               renderpass_ = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> fbos_;

  void        reinit_pass(const shared_buffer &_storage, const render_target_params &_init_params,
                          std::span<VkImageView> _images);
  void        begin_rebuild_cb(VkFramebuffer _fbo, VkCommandBuffer _cb);
  static void end_rebuild_cb(VkCommandBuffer _cb);
};

}  // namespace hut
