/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste "Jiboo" Lepesme
 * github.com/jiboo/libhut @lepesmejb +JeanBaptisteLepesme
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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

#include "libhut/mat.hpp"
#include "libhut/buffer.hpp"
#include "libhut/color.hpp"

namespace hut {

    class texture;
    class drawable;

    enum vertices_primitive_mode {
        PRIMITIVE_POINTS,
        PRIMITIVE_LINE_STRIP, PRIMITIVE_LINE_LOOP, PRIMITIVE_LINES,
        PRIMITIVE_TRIANGLE_STRIP, PRIMITIVE_TRIANGLE_FAN, PRIMITIVE_TRIANGLES
    };

    class drawable_base {
    public:
        class factory {
        public:
            virtual factory& pos(std::shared_ptr<buffer>, size_t offset, size_t stride) = 0;
            virtual factory& transform(const mat4&) = 0;

            virtual factory& opacity(const float&) = 0;

            virtual factory& col(const vec4&, blend_mode = BLEND_NONE) = 0;
            virtual factory& col(const vec4*, blend_mode = BLEND_NONE) = 0;
            virtual factory& col(std::shared_ptr<buffer>, size_t offset, size_t stride, blend_mode = BLEND_NONE) = 0;

            virtual factory& tex(std::shared_ptr<texture>, std::shared_ptr<buffer>, size_t offset, size_t stride, blend_mode = BLEND_NONE) = 0;
            virtual factory& tex(std::shared_ptr<texture>, const float*, blend_mode = BLEND_NONE) = 0;

            /** Advanced command, mainly used to render text from multiple texture atlases */
            virtual factory& multitex(std::initializer_list<std::shared_ptr<texture>>, std::shared_ptr<buffer>, size_t texindex_offset, size_t texcoord_offset, size_t stride, blend_mode = BLEND_NONE) = 0;

            virtual factory& gradient_linear(float angle, uint32_t from, uint32_t to, blend_mode = BLEND_NONE) = 0;

            virtual std::shared_ptr<drawable> compile(vertices_primitive_mode mode, size_t count) = 0;
            virtual std::shared_ptr<drawable> compile(vertices_primitive_mode mode, std::shared_ptr<buffer>, size_t offset, size_t count) = 0;
        };
    };

} // namespace hut

#ifdef HUT_WAYLAND

#include "libhut/egl/drawable.hpp"

#endif