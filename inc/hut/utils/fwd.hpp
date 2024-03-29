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

#include <memory>

namespace hut {

namespace details {
struct buffer_page_data;
}  // namespace details

class atlas;
class buffer;
class display;
class image;
class offscreen;
class render_target;
class sampler;
class subimage;
class window;

using shared_atlas    = std::shared_ptr<atlas>;
using shared_buffer   = std::shared_ptr<buffer>;
using shared_image    = std::shared_ptr<image>;
using shared_sampler  = std::shared_ptr<sampler>;
using shared_subimage = std::shared_ptr<subimage>;

template<typename TIndice, typename TVertexRefl, typename TFragRefl, typename... TExtraAttachments>
class pipeline;

template<typename TContained, typename TParent>
class suballoc;
template<typename TContained, typename TParent>
using shared_suballoc = std::shared_ptr<suballoc<TContained, TParent>>;
template<typename TContained>
using buffer_suballoc = suballoc<TContained, details::buffer_page_data>;
template<typename TContained>
using shared_buffer_suballoc = std::shared_ptr<buffer_suballoc<TContained>>;

template<typename TContained, typename TParent>
class updator;
template<typename TContained>
using buffer_updator = updator<TContained, details::buffer_page_data>;

}  // namespace hut
