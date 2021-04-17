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

#include "hut/binpacks.hpp"
#include "hut/image.hpp"

namespace hut {

class atlas_pool {
  friend class font;
  template<typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments> friend class pipeline;

 private:
  constexpr static u16vec2 padding = {1, 1};

 public:
  class subimage {
    friend class atlas_pool;
   private:
    atlas_pool *pool_ = nullptr;
    uint page_ {};
    u16vec4 bounds_ {};

   public:
    subimage() = default;
    subimage(atlas_pool *_pool, uint _page, uvec4 _bounds) : pool_(_pool), page_(_page), bounds_(_bounds) {}
    subimage(const subimage&) = delete;
    subimage(subimage &&_other) noexcept {
      *this = std::move(_other);
    }
    ~subimage();

    subimage &operator=(subimage &&_other) noexcept {
      pool_ = _other.pool_;
      page_ = _other.page_;
      bounds_ = _other.bounds_;
      _other.reset();
      return *this;
    }
    explicit operator bool() const { return valid(); }

    void reset() {
      pool_ = nullptr;
    }

    [[nodiscard]] bool valid() const {
      return pool_ != nullptr;
    }

    [[nodiscard]] uint page() const {
      return page_;
    }

    [[nodiscard]] vec4 texcoords() const {
      vec4 result;
      uvec2 atlas_size = pool_->params_.size_;
      result[0] = float(bounds_[0]) / atlas_size.x;
      result[1] = float(bounds_[1]) / atlas_size.y;
      result[2] = float(bounds_[2]) / atlas_size.x;
      result[3] = float(bounds_[3]) / atlas_size.y;
      return result;
    }

    void update(u16vec4 _bounds, span<const u8> _data, uint _dataPitch) {
      auto update_size = bbox_size(_bounds);
      auto update_origin = bbox_origin(_bounds);
      auto subimage_origin = bbox_origin(bounds_);
      auto image_bounds = make_bbox_with_origin_size(subimage_origin + update_origin, update_size);
      pool_->pages_[page_].image_->update({image_bounds}, _data, _dataPitch);
    }

    void update(span<const u8> _data, uint _dataPitch) {
      update(bounds_, _data, _dataPitch);
    }

    image::updator prepare_update(u16vec4 _bounds) {
      auto update_size = bbox_size(_bounds);
      auto update_origin = bbox_origin(_bounds);
      auto subimage_origin = bbox_origin(bounds_);
      auto image_bounds = make_bbox_with_origin_size(subimage_origin + update_origin, update_size);
      return pool_->pages_[page_].image_->prepare_update({image_bounds});
    }

    image::updator prepare_update() {
      return pool_->pages_[page_].image_->prepare_update({bounds_});
    }
  };

  atlas_pool(display &_display, const image_params &_params);

  shared_image image(uint _page) { return pages_[_page].image_; }
  size_t page_count() { return pages_.size(); }
  u16vec2 size() { return params_.size_; }

  std::shared_ptr<subimage> alloc(const u16vec2 &_bounds);
  std::shared_ptr<subimage> pack(const u16vec2 &_bounds, span<const u8> _data, uint _src_row_pitch);

 private:
  void add_page();
  void free(subimage &_ref);

  struct page_data {
    shared_image image_;
    binpack::shelve<u16, binpack::shelve_separator_align<u16, 8>> packer_;

    page_data(const shared_image &_image) : image_(_image), packer_(_image->size()) {}
  };

  display &display_;
  image_params params_;
  std::vector<page_data> pages_;
};

using shared_atlas = std::shared_ptr<atlas_pool>;
using shared_subimage = std::shared_ptr<atlas_pool::subimage>;

}  // namespace hut
