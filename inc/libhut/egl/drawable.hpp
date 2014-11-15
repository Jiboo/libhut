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
#include <memory>

#include "libhut/drawable.hpp"

namespace hut {

    class drawable : public base_drawable {
        friend class window;
    protected:
        struct attrib {
            GLuint index;
            GLint size;
            GLenum type;
            GLboolean normalized;
            GLsizei stride;
            const GLvoid* pointer;
        };

    public:
        class factory : public base_drawable::factory {
        public:
            virtual base_drawable::factory& pos(std::shared_ptr<buffer>, size_t offset, size_t stride);
            virtual base_drawable::factory& transform(const mat4&);

            virtual base_drawable::factory& opacity(const float&);

            virtual base_drawable::factory& col(const vec4&, blend_mode = BLEND_OVER);
            virtual base_drawable::factory& col(const vec4*, blend_mode = BLEND_OVER);
            virtual base_drawable::factory& col(std::shared_ptr<buffer>, size_t offset, size_t stride, blend_mode = BLEND_OVER);

            virtual base_drawable::factory& tex(std::shared_ptr<texture>, std::shared_ptr<buffer>, size_t offset, size_t stride, blend_mode = BLEND_OVER);
            virtual base_drawable::factory& tex(std::shared_ptr<texture>, const float*, blend_mode = BLEND_OVER);
            virtual base_drawable::factory& multitex(std::initializer_list<std::shared_ptr<texture>>, std::shared_ptr<buffer>, size_t texindex_offset, size_t texcoord_offset, size_t stride, blend_mode = BLEND_NONE);

            virtual base_drawable::factory& gradient_linear(float angle, uint32_t from, uint32_t to, blend_mode = BLEND_OVER);

            virtual std::shared_ptr<drawable> compile(vertices_primitive_mode mode, size_t count);
            virtual std::shared_ptr<drawable> compile(vertices_primitive_mode mode, std::shared_ptr<buffer>, size_t offset, size_t count);

        protected:
            std::map<std::string, std::tuple<GLint, const GLfloat*>> uniformsMatrix4fv;
            std::map<std::string, std::tuple<GLint, const vec4*>> uniforms4f;
            std::map<std::string, std::tuple<GLint, const GLfloat*>> uniforms1f;
            std::map<std::string, std::tuple<GLint, GLenum, GLuint>> uniforms_texture;
            std::map<std::string, std::tuple<GLint, std::vector<std::pair<GLenum, GLuint>>>> uniforms_textures;
            size_t bound_textures = 0;

            std::map<std::string, std::tuple<std::string, attrib, GLuint>> attributes;
            std::map<std::string, std::tuple<std::string, std::string>> varryings;

            std::string outPos;
            std::string outColor;

            blend_mode first_blend_mode;

            GLuint compile_vertex_shader();
            GLuint compile_fragment_shader();
        };

    protected:
        GLuint name;

        bool has_blend;
        GLenum blend_src, blend_dst;

        GLenum primitive_mode;
        size_t primitive_count;

        GLuint indices_buffer = GL_NONE;
        size_t indices_offset;

        std::vector<std::tuple<GLint, const GLfloat*>> uniformsMatrix4fv;
        std::vector<std::tuple<GLint, const vec4*>> uniforms4f;
        std::vector<std::tuple<GLint, const GLfloat*>> uniforms1f;
        std::vector<std::tuple<GLint, GLenum, GLuint>> uniforms_texture;
        std::vector<std::tuple<GLint, std::vector<std::pair<GLenum, GLuint>>>> uniforms_textures;

        std::vector<std::tuple<attrib, GLuint>> attributes;

        void draw() const;
    };

} // namespace hut
