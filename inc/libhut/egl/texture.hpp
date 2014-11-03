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

#include "libhut/texture.hpp"
#include <GLES2/gl2.h>

namespace hut {

    class texture : public base_texture {
        friend class drawable;
    protected:
        GLuint name;
        bool has_mipmaps;

    public:
        texture(const uivec2 &size, const pixel_format& data_format, void* data, const pixel_format& internal_format, bool enable_mipmaps = true)
                : base_texture(size, size, internal_format), has_mipmaps(enable_mipmaps) {
            GLint gl_internal_format;
            switch(internal_format) {
                case A8:        gl_internal_format = GL_ALPHA; break;
                case L8:        gl_internal_format = GL_LUMINANCE; break;
                case LA88:      gl_internal_format = GL_LUMINANCE_ALPHA; break;
                case RGB565:    gl_internal_format = GL_RGB; break;
                case RGBA4444:
                case RGBA5551:
                case RGBA8888:  gl_internal_format = GL_RGBA; break;
            }

            GLenum gl_format;
            GLenum gl_type;
            switch(data_format) {
                case A8:        gl_type = GL_UNSIGNED_BYTE; gl_format = GL_ALPHA; break;
                case L8:        gl_type = GL_UNSIGNED_BYTE; gl_format = GL_LUMINANCE; break;
                case LA88:      gl_type = GL_UNSIGNED_BYTE; gl_format = GL_LUMINANCE_ALPHA; break;
                case RGB565:    gl_type = GL_UNSIGNED_SHORT_5_6_5; gl_format = GL_RGB; break;
                case RGBA4444:  gl_type = GL_UNSIGNED_SHORT_4_4_4_4; gl_format = GL_RGBA; break;
                case RGBA5551:  gl_type = GL_UNSIGNED_SHORT_5_5_5_1; gl_format = GL_RGBA; break;
                case RGBA8888:  gl_type = GL_UNSIGNED_BYTE; gl_format = GL_RGBA; break;
            }

            glGenTextures(1, &name);

            glBindTexture(GL_TEXTURE_2D, name);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, size[0], size[1], 0, gl_format, gl_type, data);

            if(has_mipmaps)
                glGenerateMipmap(GL_TEXTURE_2D);

            glBindTexture(GL_TEXTURE_2D, GL_NONE);
        }

        virtual void filter(bool enable) {
            glBindTexture(GL_TEXTURE_2D, name);
            const GLenum val = has_mipmaps ? (enable ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST) : (enable ? GL_LINEAR : GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, val);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, val);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
        }

        virtual ~texture() {
            glDeleteTextures(1, &name);
        }

        virtual void update(const uivec2 &offset, const uivec2 &size, const pixel_format& data_format, void* data) {
            GLenum gl_format;
            GLenum gl_type;
            switch(data_format) {
                case A8:        gl_type = GL_UNSIGNED_BYTE; gl_format = GL_ALPHA; break;
                case L8:        gl_type = GL_UNSIGNED_BYTE; gl_format = GL_LUMINANCE; break;
                case LA88:      gl_type = GL_UNSIGNED_BYTE; gl_format = GL_LUMINANCE_ALPHA; break;
                case RGB565:    gl_type = GL_UNSIGNED_SHORT_5_6_5; gl_format = GL_RGB; break;
                case RGBA4444:  gl_type = GL_UNSIGNED_SHORT_4_4_4_4; gl_format = GL_RGBA; break;
                case RGBA5551:  gl_type = GL_UNSIGNED_SHORT_5_5_5_1; gl_format = GL_RGBA; break;
                case RGBA8888:  gl_type = GL_UNSIGNED_BYTE; gl_format = GL_RGBA; break;
            }

            glBindTexture(GL_TEXTURE_2D, name);
            glTexSubImage2D(GL_TEXTURE_2D, 0, offset[0], offset[1], size[0], size[1], gl_format, gl_type, data);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
        }
    };

} //namespace hut
