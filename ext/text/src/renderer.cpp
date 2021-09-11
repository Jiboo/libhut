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

#include <string_view>
#include <utility>

#include "hut/utils/profiling.hpp"
#include "hut/utils/utf.hpp"

#include "hut/display.hpp"

#include "hut/text/renderer.hpp"

using namespace hut;
using namespace hut::text;

ivec2 encode_atlas_page_xy(vec2 _in, uint _atlas_page) {
  assert(_atlas_page < 4);
  switch (_atlas_page) {
    case 0:
      _in.x *= +1;
      _in.y *= +1;
      break;
    case 1:
      _in.x *= +1;
      _in.y *= -1;
      break;
    case 2:
      _in.x *= -1;
      _in.y *= +1;
      break;
    case 3:
      _in.x *= -1;
      _in.y *= -1;
      break;
    default:
      throw std::runtime_error("not enough bits to encode atlas page in uv signs");
  }
  return packSnorm<i16>(_in);
}

renderer::renderer(render_target &_target, shared_buffer _buffer, const shared_font &_font,
                   const shared_ubo &_ubo, shared_atlas _atlas, const shared_sampler &_sampler,
                   renderer_params _params)
    : pipeline_(_target, _params)
    , buffer_(std::move(_buffer))
    , atlas_(std::move(_atlas))
    , shaper_(_font) {
  constexpr u16 max_atlas_bounds = 32 * 1024;
  assert(atlas_->size().x <= max_atlas_bounds && atlas_->size().y <= max_atlas_bounds);
  if (_params.initial_mesh_store_size_ > 0 && _params.initial_draw_store_size_ > 0)
    grow(_params.initial_mesh_store_size_, _params.initial_draw_store_size_);
  pipeline_.write(0, _ubo, atlas_, _sampler);
}

renderer::mesh_store::mesh_store(renderer *_parent, uint _size)
    : vertices_(_parent->buffer_->allocate<vertex>(_size * 4))
    , indices_(_parent->buffer_->allocate<index_t>(_size * 6))
    , suballocator_(_size) {
}

renderer::draw_store::draw_store(renderer *_parent, uint _size)
    : instances_(_parent->buffer_->allocate<instance>(_size))
#ifndef HUT_TEXT_DEBUG_NO_INDIRECT
    , commands_(_parent->buffer_->allocate<VkDrawIndexedIndirectCommand>(_size))
#else
    , commands_(_size)
#endif
    , suballocator_(_size) {
}

renderer::words_info::words_info(std::span<const std::u8string_view> _words)
    : texts_(_words) {
  auto count        = _words.size();
  hashes_           = std::make_unique<string_hash[]>(count);
  codepoints_       = std::make_unique<uint[]>(count);
  total_codepoints_ = 0;
  for (uint i = 0; i < count; i++) {
    hashes_[i]            = std::hash<std::u8string_view>{}(_words[i]);
    const auto codepoints = utf8codepointcount(_words[i]);
    codepoints_[i]        = codepoints;
    total_codepoints_ += codepoints;
  }
}

using flimits = std::numeric_limits<float>;

renderer::word::word(renderer *_parent, batch &_batch, uint _alloc, uint _codepoints, std::u8string_view _text)
    : alloc_(_alloc)
    , glyphs_(0)
    , bbox_(flimits::max(), flimits::max(), flimits::min(), flimits::min()) {
  const auto vertices_offset = sizeof(vertex) * 4 * _alloc;
  const auto indices_offset  = sizeof(index_t) * 6 * _alloc;
  const auto vertices_size   = sizeof(vertex) * 4 * _codepoints;
  const auto indices_size    = sizeof(index_t) * 6 * _codepoints;

  auto vupdator = _batch.mstore_.vertices_->update_raw(vertices_offset, vertices_size);
  auto iupdator = _batch.mstore_.indices_->update_raw(indices_offset, indices_size);

  auto *vertices = reinterpret_cast<vertex *>(vupdator.staging().data());
  auto *indices  = reinterpret_cast<index_t *>(iupdator.staging().data());

  auto callback = [vertices, indices, this](uint _index, i16vec4 _coords, vec4 _uv, uint atlas_page) {
    vertices[_index * 4 + 0] = {{_coords.x, _coords.y}, encode_atlas_page_xy({_uv.x, _uv.y}, atlas_page)};
    vertices[_index * 4 + 1] = {{_coords.x, _coords.w}, encode_atlas_page_xy({_uv.x, _uv.w}, atlas_page)};
    vertices[_index * 4 + 2] = {{_coords.z, _coords.y}, encode_atlas_page_xy({_uv.z, _uv.y}, atlas_page)};
    vertices[_index * 4 + 3] = {{_coords.z, _coords.w}, encode_atlas_page_xy({_uv.z, _uv.w}, atlas_page)};

    indices[_index * 6 + 0] = _index * 4 + 0;
    indices[_index * 6 + 1] = _index * 4 + 1;
    indices[_index * 6 + 2] = _index * 4 + 2;
    indices[_index * 6 + 3] = _index * 4 + 2;
    indices[_index * 6 + 4] = _index * 4 + 1;
    indices[_index * 6 + 5] = _index * 4 + 3;

    bbox_.x = std::min(bbox_.x, _coords.x);
    bbox_.y = std::min(bbox_.y, _coords.y);
    bbox_.z = std::max(bbox_.z, _coords.z);
    bbox_.w = std::max(bbox_.w, _coords.w);
    glyphs_++;
  };
  _parent->shaper_.shape(_parent->atlas_, _text, callback);
}

void renderer::draw(VkCommandBuffer _buff) {
  pipeline_.bind_pipeline(_buff);
  pipeline_.bind_descriptor(_buff, 0);
  for (const auto &batch : batches_) {
    pipeline_.bind_indices(_buff, batch.mstore_.indices_);
    pipeline_.bind_vertices(_buff, batch.mstore_.vertices_);
    pipeline_.bind_instances(_buff, batch.dstore_.instances_);
#ifndef HUT_TEXT_DEBUG_NO_INDIRECT
    pipeline_.draw_indexed(_buff, batch.dstore_.commands_, batch.dstore_.suballocator_.upper_bound(), sizeof(VkDrawIndexedIndirectCommand));
#else
    for (uint i = 0; i < batch.dstore_.suballocator_.upper_bound(); i++) {
      auto &command = batch.dstore_.commands_[i];
      pipeline_.draw_indexed(_buff, command.indexCount, command.instanceCount, command.firstIndex, command.vertexOffset, command.firstInstance);
    }
#endif
  }
}

renderer::batch &renderer::grow(uint _mesh_store_size, uint _draw_store_size) {
  auto mesh_back_size  = batches_.empty() ? 0 : batches_.back().mstore_.suballocator_.capacity();
  auto draw_back_size  = batches_.empty() ? 0 : batches_.back().dstore_.suballocator_.capacity();
  auto mesh_store_size = std::max(_mesh_store_size, mesh_back_size * 2);
  auto draw_store_size = std::max(_draw_store_size, draw_back_size * 2);
  assert(mesh_store_size > 0 && draw_store_size > 0);
  return batches_.emplace_back(batch{this, mesh_store_size, draw_store_size});
}

renderer::shared_suballoc<renderer::instance> renderer::allocate(std::span<const std::u8string_view> _words) {
  words_info winfo{_words};
  auto       best_batch_id = find_best_fit(winfo);
  auto &     best_batch    = batches_[best_batch_id];

  auto draw_alloc = best_batch.dstore_.suballocator_.pack(_words.size());
  assert(draw_alloc);
  shared_suballoc<instance> result = std::make_shared<suballoc<instance>>(this, best_batch_id, *draw_alloc, _words.size() * sizeof(instance));
  result->bboxes_                  = std::make_unique<i16vec4[]>(_words.size());

#ifndef HUT_TEXT_DEBUG_NO_INDIRECT
  auto cupdator = best_batch.dstore_.commands_->update_raw(sizeof(VkDrawIndexedIndirectCommand) * *draw_alloc,
                                                           sizeof(VkDrawIndexedIndirectCommand) * _words.size());
#endif

  for (uint i = 0; i < winfo.size(); i++) {
    const auto hash       = winfo.hashes_[i];
    const auto codepoints = winfo.codepoints_[i];
    auto       it         = best_batch.cache_.find(hash);
    if (it == best_batch.cache_.end()) {
      auto word_alloc = best_batch.mstore_.suballocator_.pack(codepoints);
      assert(word_alloc);
      auto emplaced = best_batch.cache_.emplace(hash, word{this, best_batch, *word_alloc, codepoints, winfo.texts_[i]});
      assert(emplaced.second);
      it = emplaced.first;
    }
    it->second.ref_count_++;
    result->bboxes_[i] = it->second.bbox_;

#ifndef HUT_TEXT_DEBUG_NO_INDIRECT
    VkDrawIndexedIndirectCommand *cptr = reinterpret_cast<VkDrawIndexedIndirectCommand *>(cupdator.staging().data()) + i;
#else
    VkDrawIndexedIndirectCommand *cptr = best_batch.dstore_.commands_.data() + *draw_alloc + i;
#endif

    cptr->firstInstance = *draw_alloc + i;
    cptr->instanceCount = 1;
    cptr->firstIndex    = it->second.alloc_ * 6;
    cptr->indexCount    = 6 * it->second.glyphs_;
    cptr->vertexOffset  = (int)it->second.alloc_ * 4;
  }

  result->hashes_ = std::move(winfo.hashes_);
  return result;
}

size_t renderer::find_best_fit(const words_info &_winfo) {
  uint best_batch = -1;
  uint best_score = 0;
  for (uint ib = 0; ib < batches_.size(); ib++) {
    uint  score = 0;
    auto &b     = batches_[ib];
    if (!b.dstore_.suballocator_.try_fit(_winfo.size()))
      continue;
    for (uint iw = 0; iw < _winfo.size(); iw++) {
      auto it = b.cache_.find(_winfo.hashes_[iw]);
      if (it != b.cache_.end())
        score += it->second.glyphs_;
    }
    const uint needed_codepoints = _winfo.total_codepoints_ - score;
    if (!b.mstore_.suballocator_.try_fit(needed_codepoints))
      continue;
    score += b.dstore_.suballocator_.free() * 8 + b.mstore_.suballocator_.free();
    if (score > best_score) {
      best_batch = ib;
      best_score = score;
    }
  }

  if (best_batch == -1) {
    best_batch = batches_.size();
    grow(_winfo.total_codepoints_, _winfo.size());
  }

  return best_batch;
}

suballoc_raw::updator renderer::renderer_suballoc::update_raw(uint _offset_bytes, uint _size_bytes) {
  assert(parent_);
  auto total_offset_bytes = offset_bytes() + _offset_bytes;
  return parent_->batches_[batch_].dstore_.instances_->update_raw(total_offset_bytes, _size_bytes);
}

void renderer::renderer_suballoc::finalize(const suballoc_raw::updator &_updator) {
  assert(parent_);
  return parent_->batches_[batch_].dstore_.instances_->finalize(_updator);
}

void renderer::renderer_suballoc::zero_raw(uint _offset_bytes, uint _size_bytes) {
  assert(parent_);
  auto total_offset_bytes = offset_bytes() + _offset_bytes;
  return parent_->batches_[batch_].dstore_.instances_->zero_raw(total_offset_bytes, _size_bytes);
}

VkBuffer renderer::renderer_suballoc::underlying_buffer() const {
  assert(parent_);
  return parent_->batches_[batch_].dstore_.instances_->underlying_buffer();
}

u8 *renderer::renderer_suballoc::existing_mapping() {
  assert(parent_);
  return parent_->batches_[batch_].dstore_.instances_->existing_mapping() + offset_bytes();
}

void renderer::renderer_suballoc::release() {
  assert(parent_);
  auto &batch  = parent_->batches_[batch_];
  auto &dstore = batch.dstore_;
  auto &mstore = batch.mstore_;
#ifndef HUT_TEXT_DEBUG_NO_INDIRECT
  dstore.commands_->zero_raw(offset_bytes(), size_bytes());
#else
  auto offset_elements = offset_bytes() / sizeof(instance);
  auto size_elements   = size_bytes() / sizeof(instance);
  std::fill(dstore.commands_.data() + offset_elements, dstore.commands_.data() + offset_elements + size_elements, VkDrawIndexedIndirectCommand{0});
#endif
  dstore.suballocator_.offer(offset_bytes());
  auto &cache       = batch.cache_;
  auto  words_count = size_bytes() / sizeof(instance);
  for (uint i = 0; i < words_count; i++) {
    const auto hash = hashes_[i];
    auto       it   = cache.find(hash);
    assert(it != cache.end());
    auto &word = it->second;
    word.ref_count_--;
    if (word.ref_count_ == 0) {
      mstore.suballocator_.offer(word.alloc_);
      /*mstore.vertices_->zero(word.alloc_ * 4, word.codepoints_ * 4);
      mstore.indices_->zero(word.alloc_ * 6, word.codepoints_ * 6);*/
      // NOTE JBL: Assumes we don't need to clear, the commands were removed..
      // TODO JBL: Also decrease refcount of glyphs cache in shaper?
      auto result = cache.erase(hash);
      assert(result == 1);
    }
  }
  parent_ = nullptr;
}
