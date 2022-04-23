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

#include "hut/utils/length.hpp"

#include "hut/image.hpp"
#include "hut/subimage.hpp"

namespace hut {

class atlas {
  template<typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments>
  friend class pipeline;

  constexpr static u16vec2_px PADDING = {1, 1};

 public:
  atlas(display &_display, const image_params &_params);

  shared_image             page(uint _index) { return pages_[_index].image_; }
  [[nodiscard]] size_t     page_count() const { return pages_.size(); }
  [[nodiscard]] u16vec2_px size() const { return params_.size_; }
  [[nodiscard]] VkFormat   format() const { return params_.format_; }

  shared_subimage alloc(const u16vec2_px &_bounds);
  shared_subimage pack(const u16vec2_px &_bounds, std::span<const u8> _data, uint _src_row_pitch);
  image::updator  update(const subimage &_sub, const u16vec4_px &_bounds);
  void            free(subimage &&_sub);

 private:
  void add_page();

  struct page_data {
    using packer_t = binpack::shelve<u16, binpack::shelve_separator_align<u16, 8>>;

    shared_image image_;
    packer_t     packer_;

    explicit page_data(const shared_image &_image)
        : image_(_image)
        , packer_(_image->size()) {}
  };

  display               &display_;
  image_params           params_;
  std::vector<page_data> pages_;
};

}  // namespace hut
