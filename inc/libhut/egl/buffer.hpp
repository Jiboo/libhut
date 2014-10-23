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

#include <GLES2/gl2.h>

#include "libhut/buffer.hpp"

namespace hut {

    class buffer : public base_buffer {
        friend class drawable;
    protected:
        GLuint name;
        size_t buf_size;

    public:
        buffer(const uint8_t* data, size_t size, buffer_hint hint = BSTREAM)
                : base_buffer(size) {
            glGenBuffers(1, &name);
            glBindBuffer(GL_ARRAY_BUFFER, name);
            GLenum gl_hint;
            switch(hint) {
                case BSTATIC: gl_hint = GL_STATIC_DRAW; break;
                case BDYNAMIC: gl_hint = GL_DYNAMIC_DRAW; break;
                case BSTREAM: gl_hint = GL_STREAM_DRAW; break;
            }
            glBufferData(GL_ARRAY_BUFFER, size, data, gl_hint);
        }

        virtual ~buffer() {
            glDeleteBuffers(1, &name);
        }

        virtual void update(const uint8_t* data, size_t offset, size_t count) {
            glBindBuffer(GL_ARRAY_BUFFER, name);
            glBufferSubData(GL_ARRAY_BUFFER, offset, count, data);
        }
    };

} //namespace hut
