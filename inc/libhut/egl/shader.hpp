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

#include "libhut/shader.hpp"

namespace hut {

    class shader : public shader_base {
    public:
        class factory : public shader_base::factory {
        public:
            virtual factory& transform(const mat3&);

            virtual factory& opacity(const float&);

            virtual factory& col(const uint32_t&, blend_mode = BOVER);
            virtual factory& col(const uint32_t*, blend_mode = BOVER);
            virtual factory& col(const buffer<uint32_t>&, size_t offset, size_t stride, size_t count, blend_mode = BOVER);

            virtual factory& tex(const texture&, const buffer<float>&, size_t offset, size_t stride, blend_mode = BOVER);
            virtual factory& tex(const texture&, const float*, blend_mode = BOVER);

            virtual factory& gradient_linear(float angle, uint32_t from, uint32_t to, blend_mode = BOVER);

            virtual shader compile();

        protected:
            std::map<std::string, std::tuple<GLint, const GLfloat *>> uniformsMatrix3fv;
            std::map<std::string, std::tuple<GLint, const GLint &>> uniforms4f;
            std::map<std::string, std::tuple<GLint, const GLfloat &>> uniforms1f;
            std::map<std::string, std::tuple<GLint, GLenum, GLboolean, GLsizei, const GLvoid*>> attributes;
            std::map<std::string, std::tuple<const buffer<uint32_t>&, GLint, GLenum, GLboolean, GLsizei, const GLvoid*>> bound_attributes;
            std::map<std::string, std::tuple<std::string, std::string>> varryings;
            blend_mode first_mode;

            std::string outPos = "__hut_pos";
            std::string outColor = "";

            GLuint compile_vertex_shader();
            GLuint compile_fragment_shader();
        };

    protected:
        shader() {}
        GLuint name;
        blend_mode first_mode;
        std::vector<std::tuple<GLint, const GLfloat *>> uniformsMatrix3fv;
        std::vector<std::tuple<GLint, const GLint &>> uniforms4i;
        std::vector<std::tuple<GLint, const GLfloat &>> uniforms1f;
    };

} // namespace hut
