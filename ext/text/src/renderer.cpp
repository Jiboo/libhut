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

#include "hut/text/renderer.hpp"

#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "hut/utils/utf.hpp"

#include "hut/suballoc.hpp"
#include "hut/display.hpp"

namespace hut::text {

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
    default: throw std::runtime_error("not enough bits to encode atlas page in uv signs");
  }
  assert(_in.x < NUMAX<i16>);
  assert(_in.x > NUMIN<i16>);
  assert(_in.y < NUMAX<i16>);
  assert(_in.y > NUMIN<i16>);
  return packSnorm<i16>(_in);
}

renderer::renderer(render_target &_target, shared_buffer _buffer, const shared_font &_font, const shared_ubo &_ubo,
                   shared_atlas _atlas, const shared_sampler &_sampler, renderer_params _params)
    : pipeline_(_target, _params)
    , buffer_(std::move(_buffer))
    , atlas_(std::move(_atlas))
    , shaper_(_font) {
  const auto &features   = _target.parent().features();
  use_indirect_fallback_ = features.multiDrawIndirect != VK_TRUE || features.drawIndirectFirstInstance != VK_TRUE;
  if (use_indirect_fallback_) {
    std::cout << "[hut] text renderer had to fallback due to missing feature "
              << " (multiDrawIndirect: " << features.multiDrawIndirect
              << ", drawIndirectFirstInstance: " << features.drawIndirectFirstInstance << ")" << std::endl;
  }
  const auto &features12 = _target.parent().features12();
  if (features12.shaderSampledImageArrayNonUniformIndexing == VK_FALSE
      || features12.descriptorBindingPartiallyBound == VK_FALSE)
    throw std::runtime_error("vulkan device does not meet minimum requirements for text renderer");

  constexpr u16_px MAX_ATLAS_BOUNDS = 32_px * 1024;
  assert(atlas_->size().x <= MAX_ATLAS_BOUNDS && atlas_->size().y <= MAX_ATLAS_BOUNDS);
  if (_params.initial_mesh_store_size_ > 0 && _params.initial_draw_store_size_ > 0)
    grow(_params.initial_mesh_store_size_, _params.initial_draw_store_size_);
  pipeline_.write(0, _ubo, atlas_, _sampler);
}

renderer::words_info::words_info(std::span<const std::u8string_view> _words)
    : texts_(_words) {
  auto count        = _words.size();
  hashes_           = std::make_unique<string_hash[]>(count);
  codepoints_       = std::make_unique<uint[]>(count);
  total_codepoints_ = 0;
  for (uint i = 0; i < count; i++) {
    hashes_[i]            = std::hash<std::u8string_view>{}(_words[i]);
    const auto codepoints = utf8_codepoint_count(_words[i]);
    codepoints_[i]        = codepoints;
    total_codepoints_ += codepoints;
  }
}

void renderer::draw(VkCommandBuffer _buff) {
  pipeline_.bind_pipeline(_buff);
  pipeline_.bind_descriptor(_buff, 0);
  for (const auto &batch : batches_) {
    if (batch.dstore_.suballocator_.empty())
      continue;

    pipeline_.bind_indices(_buff, batch.mstore_.indices_);
    pipeline_.bind_vertices(_buff, batch.mstore_.vertices_);
    pipeline_.bind_instances(_buff, batch.dstore_.instances_);

    constexpr bool OPTIMIZE = true;
    const uint     lower    = OPTIMIZE ? batch.dstore_.suballocator_.lower_bound() : 0;
    const uint upper = OPTIMIZE ? batch.dstore_.suballocator_.upper_bound() : batch.dstore_.suballocator_.capacity();
    assert(upper >= lower);
    if (!use_indirect_fallback_) {
      pipeline_.draw_indexed(_buff, batch.dstore_.commands_, upper - lower, lower,
                             sizeof(VkDrawIndexedIndirectCommand));
    } else {
      for (uint i = lower; i < upper; i++) {
        auto &command = batch.dstore_.commands_fallback_[i];
        pipeline_.draw_indexed(_buff, command.indexCount, command.instanceCount, command.firstIndex,
                               command.vertexOffset, command.firstInstance);
      }
    }
  }
}

details::batch &renderer::grow(uint _mesh_store_size, uint _draw_store_size) {
  auto mesh_back_size = batches_.empty() ? 0 : batches_.back().mstore_.suballocator_.capacity();
  auto draw_back_size = batches_.empty() ? 0 : batches_.back().dstore_.suballocator_.capacity();

  auto mesh_store_size = std::max(_mesh_store_size, mesh_back_size * 2);
  auto draw_store_size = std::max(_draw_store_size, draw_back_size * 2);

  assert(mesh_store_size > 0 && draw_store_size > 0);
  return batches_.emplace_back(this, mesh_store_size, draw_store_size);
}

words_holder renderer::allocate(std::span<const std::u8string_view> _words) {
  words_info winfo{_words};
  auto      &best_batch = find_best_fit(winfo);

  auto draw_alloc = best_batch.dstore_.suballocator_.pack(_words.size());
  assert(draw_alloc);

  if (!use_indirect_fallback_) {
    auto  cupdator     = best_batch.dstore_.commands_->update(*draw_alloc, _words.size());
    auto *commands_ptr = reinterpret_cast<VkDrawIndexedIndirectCommand *>(cupdator.staging().data());
    return prepare_commands(_words, winfo, best_batch, *draw_alloc, commands_ptr);
  } else {
    auto *commands_ptr = best_batch.dstore_.commands_fallback_.get() + *draw_alloc;
    return prepare_commands(_words, winfo, best_batch, *draw_alloc, commands_ptr);
  }
}

words_holder renderer::prepare_commands(const std::span<const std::u8string_view> &_words, renderer::words_info &_winfo,
                                        details::batch &_batch, uint _alloc,
                                        VkDrawIndexedIndirectCommand *_commands_ptr) {
  words_holder result{&_batch, uint(_alloc * sizeof(instance)), uint(_words.size() * sizeof(instance))};
  result.bboxes_ = std::make_unique<i16vec4_px[]>(_words.size());
  for (uint i = 0; i < _winfo.size(); i++) {
    const auto hash       = _winfo.hashes_[i];
    const auto codepoints = _winfo.codepoints_[i];
    auto       it         = _batch.cache_.find(hash);
    if (it == _batch.cache_.end()) {
      auto word_alloc = _batch.mstore_.suballocator_.pack(codepoints);
      assert(word_alloc);
      auto emplaced
          = _batch.cache_.emplace(hash, details::word{this, _batch, *word_alloc, codepoints, _winfo.texts_[i]});
      assert(emplaced.second);
      it = emplaced.first;
    }
    it->second.ref_count_++;
    result.bboxes_[i] = it->second.bbox_;

    VkDrawIndexedIndirectCommand *cptr = _commands_ptr + i;

    cptr->firstInstance = _alloc + i;
    cptr->instanceCount = 1;
    cptr->firstIndex    = it->second.alloc_ * 6;
    cptr->indexCount    = 6 * it->second.glyphs_;
    cptr->vertexOffset  = (int)it->second.alloc_ * 4;
  }

  result.hashes_ = std::move(_winfo.hashes_);
  return result;
}

details::batch &renderer::find_best_fit(const words_info &_winfo) {
  details::batch *best_batch = nullptr;
  uint            best_score = 0;
  for (auto &b : batches_) {
    uint score = 0;
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
      best_batch = &b;
      best_score = score;
    }
  }

  if (best_batch != nullptr)
    return *best_batch;
  return grow(_winfo.total_codepoints_, _winfo.size());
}

std::span<instance> batch_updators::locate(const words_holder &_holder) {
  auto &batch_updator = updators_.at(_holder.instances_.parent());
  return std::span<instance>{batch_updator.begin(), batch_updator.end()};
}

batch_updators renderer::update_all() {
  batch_updators result;
  result.updators_.reserve(batches_.size());

  for (auto &batch : batches_)
    result.updators_.emplace(&batch, batch.dstore_.instances_->update());

  return result;
}

namespace details {

mesh_store::mesh_store(renderer *_parent, uint _size)
    : vertices_(_parent->buffer_->allocate<vertex>(_size * 4))
    , indices_(_parent->buffer_->allocate<index_t>(_size * 6))
    , suballocator_(_size) {
}

draw_store::draw_store(renderer *_parent, uint _size)
    : instances_(_parent->buffer_->allocate<instance>(_size))
    , suballocator_(_size) {
  if (!_parent->use_indirect_fallback_) {
    commands_ = _parent->buffer_->allocate<VkDrawIndexedIndirectCommand>(_size);
    commands_->zero();
  } else {
    commands_fallback_ = std::make_unique<VkDrawIndexedIndirectCommand[]>(_size);
    memset(commands_fallback_.get(), 0, sizeof(VkDrawIndexedIndirectCommand) * _size);
  }
}

word::word(renderer *_parent, batch &_batch, uint _alloc, uint _codepoints, std::u8string_view _text)
    : alloc_(_alloc)
    , bbox_(NUMAX<f32>, NUMAX<f32>, NUMIN<f32>, NUMIN<f32>) {
  const auto vertices_offset = 4 * _alloc;
  const auto vertices_size   = 4 * _codepoints;
  const auto indices_offset  = 6 * _alloc;
  const auto indices_size    = 6 * _codepoints;

  auto vupdator = _batch.mstore_.vertices_->update(vertices_offset, vertices_size);
  auto iupdator = _batch.mstore_.indices_->update(indices_offset, indices_size);

  auto *vertices = reinterpret_cast<vertex *>(vupdator.staging().data());
  auto *indices  = reinterpret_cast<index_t *>(iupdator.staging().data());

  auto callback = [vertices, indices, this](uint _index, i16vec4_px _coords, vec4 _uv, uint _atlas_page) {
    vertices[_index * 4 + 0] = {{_coords.x, _coords.y}, encode_atlas_page_xy({_uv.x, _uv.y}, _atlas_page)};
    vertices[_index * 4 + 1] = {{_coords.x, _coords.w}, encode_atlas_page_xy({_uv.x, _uv.w}, _atlas_page)};
    vertices[_index * 4 + 2] = {{_coords.z, _coords.y}, encode_atlas_page_xy({_uv.z, _uv.y}, _atlas_page)};
    vertices[_index * 4 + 3] = {{_coords.z, _coords.w}, encode_atlas_page_xy({_uv.z, _uv.w}, _atlas_page)};

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

void batch::release(text_suballoc *_suballoc) {
  if (!parent_->use_indirect_fallback_) {
    dstore_.commands_->zero(_suballoc->offset(), _suballoc->size());
  } else {
    std::fill(dstore_.commands_fallback_.get() + _suballoc->offset(),
              dstore_.commands_fallback_.get() + _suballoc->offset() + _suballoc->size(),
              VkDrawIndexedIndirectCommand{0});
  }
  dstore_.suballocator_.offer(_suballoc->offset());
}

void batch::release_words(std::span<string_hash> _hashes) {
  for (auto hash : _hashes) {
    auto it = cache_.find(hash);
    assert(it != cache_.end());
    auto &word = it->second;
    word.ref_count_--;
    if (word.ref_count_ == 0) {
      mstore_.suballocator_.offer(word.alloc_);
      // TODO JBL: Also decrease refcount of glyphs cache in shaper?
      auto result = cache_.erase(hash);
      assert(result == 1);
    }
  }
}

text_updator batch::update_raw_impl(uint _offset_bytes, uint _size_bytes) {
  return dstore_.instances_->update_raw(_offset_bytes, _size_bytes);
}

void batch::zero_raw(uint _offset_bytes, uint _size_bytes) {
  dstore_.instances_->zero_raw(_offset_bytes, _size_bytes);
}

}  // namespace details

}  // namespace hut::text
