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

#include "libhut/event.hpp"
#include "libhut/display.hpp"
#include "libhut/window.hpp"

namespace hut {

    enum buffer_hint {
        BSTATIC, BDYNAMIC, BSTREAM
    };

    template<typename T>
    class base_buffer {
    public:
        //virtual base_buffer(const T*, size_t size, buffer_hint hint = BSTREAM) = 0;
        //virtual base_buffer(const std::initializer_list<T>& list, buffer_hint hint = BSTREAM) = 0;

        virtual ~base_buffer() {

        }

        virtual size_t size() = 0;

        virtual void update(const T*, size_t offset, size_t count) = 0;
        virtual void update(const std::initializer_list<T>& list, size_t offset) = 0;
    };

} //namespace hut

#ifdef HUT_WAYLAND

#include "libhut/egl/buffer.hpp"

#endif