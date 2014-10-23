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

#include <string>
#include "libhut/optional.hpp"

#include "libhut/drawable.hpp"

namespace hut {

    class drawable : public drawable_base {
    public: //FIXME
        struct attrib {
            GLuint index;
            GLint size;
            GLenum type;
            GLboolean normalized;
            GLsizei stride;
            const GLvoid* pointer;
        };

    public:
        class factory : public drawable_base::factory {
        public:
            virtual factory& pos(const buffer&, size_t offset, size_t stride);
            virtual factory& transform(const mat4&);

            virtual factory& opacity(const float&);

            virtual factory& col(const vec4&, blend_mode = BOVER);
            virtual factory& col(const vec4*, blend_mode = BOVER);
            virtual factory& col(const buffer&, size_t offset, size_t stride, blend_mode = BOVER);

            virtual factory& tex(const texture&, const buffer&, size_t offset, size_t stride, blend_mode = BOVER);
            virtual factory& tex(const texture&, const float*, blend_mode = BOVER);

            virtual factory& gradient_linear(float angle, uint32_t from, uint32_t to, blend_mode = BOVER);

            virtual drawable compile();

        protected:
            std::map<std::string, std::tuple<GLint, const GLfloat*>> uniformsMatrix4fv;
            std::map<std::string, std::tuple<GLint, const vec4*>> uniforms4f;
            std::map<std::string, std::tuple<GLint, const GLfloat*>> uniforms1f;

            std::map<std::string, std::tuple<std::string, attrib, GLuint>> attributes;
            std::map<std::string, std::tuple<std::string, std::string>> varryings;
            blend_mode first_mode;

            std::string outPos;
            std::string outColor;

            GLuint compile_vertex_shader();
            GLuint compile_fragment_shader();
        };

    public: //FIXME
        GLuint name;
        blend_mode first_mode;

        std::vector<std::tuple<GLint, const GLfloat*>> uniformsMatrix4fv;
        std::vector<std::tuple<GLint, const vec4*>> uniforms4f;
        std::vector<std::tuple<GLint, const GLfloat*>> uniforms1f;

        std::vector<std::tuple<attrib, GLuint>> attributes;

        drawable() {
        }

        void bind();

        void unbind();
    };

} // namespace hut
