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

        GLenum gl_type() {
            return type == BUFFER_DATA ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
        }

    public:
        buffer(const uint8_t* data, size_t size, buffer_type type = BUFFER_DATA, buffer_usage_hint hint = BUFFER_USAGE_STREAM)
                : base_buffer(size, type) {
            glGenBuffers(1, &name);
            glBindBuffer(gl_type(), name);
            GLenum gl_hint;
            switch(hint) {
                case BUFFER_USAGE_STATIC: gl_hint = GL_STATIC_DRAW; break;
                case BUFFER_USAGE_DYNAMIC: gl_hint = GL_DYNAMIC_DRAW; break;
                case BUFFER_USAGE_STREAM: gl_hint = GL_STREAM_DRAW; break;
            }
            glBufferData(gl_type(), size, data, gl_hint);
        }

        virtual ~buffer() {
            glDeleteBuffers(1, &name);
        }

        virtual void update(const uint8_t* data, size_t offset, size_t count) {
            glBufferSubData(gl_type(), offset, count, data);
        }
    };

} //namespace hut
