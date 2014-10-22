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

#include "libhut/mat.hpp"
#include "libhut/buffer.hpp"

namespace hut {

    enum blend_mode {
        BNONE = -1,
        BCLEAR = 0, BSOURCE = 1, BDEST = 2, BXOR = 3,
        BATOP = 4, BOVER = 5, BIN = 6, BOUT = 7,
        BDEST_ATOP = 8, BDEST_OVER = 9, BDEST_IN = 10, BDEST_OUT = 11
    };

    class texture;
    class shader;

    class shader_base {
    public:
        class factory {
        public:
            virtual factory& transform(const mat3&) = 0;

            virtual factory& opacity(const float&) = 0;

            virtual factory& col(const uint32_t&, blend_mode = BOVER) = 0;
            virtual factory& col(const uint32_t*, blend_mode = BOVER) = 0;
            virtual factory& col(const buffer<uint32_t>&, size_t offset, size_t stride, size_t count, blend_mode = BOVER) = 0;

            virtual factory& tex(const texture&, const buffer<float>&, size_t offset, size_t stride, blend_mode = BOVER) = 0;
            virtual factory& tex(const texture&, const float*, blend_mode = BOVER) = 0;

            virtual factory& gradient_linear(float angle, uint32_t from, uint32_t to, blend_mode = BOVER) = 0;

            virtual shader compile() = 0;
        };
    };

} // namespace hut

#ifdef HUT_WAYLAND

#include "libhut/egl/shader.hpp"

#endif