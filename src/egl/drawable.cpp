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

#include <sstream>

#include "libhut/egl/drawable.hpp"

namespace hut {

    static std::string blending_name(blend_mode mode) {
        switch(mode) {
            case BLEND_NONE: return "BLEND_NONE";
            case BLEND_CLEAR: return "BLEND_CLEAR";
            case BLEND_SRC: return "BLEND_SRC";
            case BLEND_DST: return "BLEND_DST";
            case BLEND_XOR: return "BLEND_XOR";
            case BLEND_ATOP: return "BLEND_ATOP";
            case BLEND_OVER: return "BLEND_OVER";
            case BLEND_IN: return "BIN";
            case BLEND_OUT: return "BLEND_OUT";
            case BLEND_DST_ATOP: return "BLEND_DST_ATOP";
            case BLEND_DST_OVER: return "BLEND_DST_OVER";
            case BLEND_DST_IN: return "BLEND_DST_IN";
            case BLEND_DST_OUT: return "BLEND_DST_OUT";
        }
        return "BLEND_NONE";
    }

    drawable_base::factory& drawable::factory::pos(const buffer& data, size_t offset, size_t stride) {
        GLuint index = (GLuint)attributes.size();
        std::stringstream buf;
        buf << "pos_" << index;

        attrib attr;
        attr.index = index;
        attr.normalized = GL_FALSE;
        attr.pointer = (GLvoid*)offset;
        attr.size = 3;
        attr.stride = (GLsizei)stride;
        attr.type = GL_FLOAT;

        attributes.emplace(buf.str(), std::make_tuple("vec3", attr, data.name));

        std::stringstream pos;

        pos << " + vec4(" << buf.str() << ", 1)";
        outPos += pos.str();

        return *this;
    }

    drawable_base::factory& drawable::factory::transform(const mat4& ref) {
        runtime_assert(outPos != "", "Define position before transforms");

        std::stringstream name;
        name << "transform_" << uniformsMatrix4fv.size();

        uniformsMatrix4fv.emplace(name.str(), std::make_tuple(-1, ref.data->data));
        outPos += '*' + name.str();
        return *this;
    }

    drawable_base::factory& drawable::factory::opacity(const float& ref) {
        std::stringstream buf;
        buf << "opacity_" << uniforms1f.size();
        std::string name = buf.str();

        uniforms1f.emplace(name, std::make_tuple(-1, &ref));

        std::stringstream color;
        color << "blend_opacity(" << outColor << ", " << name << ")";
        outColor = color.str();

        return *this;
    }

    drawable_base::factory& drawable::factory::col(const vec4& ref, blend_mode mode) {
        std::stringstream buf;
        buf << "col_uni_" << uniforms4f.size();
        std::string name = buf.str();

        uniforms4f.emplace(name, std::make_tuple(-1, &ref));

        if(outColor.empty()) {
            first_blend_mode = mode;
            outColor = name;
        } else {
            std::stringstream color;
            color << "porterduff(" << name << ", " << outColor << ", " << blending_name(mode) << ")";
            outColor = color.str();
        }

        return *this;
    }

    drawable_base::factory& drawable::factory::col(const vec4* colors, blend_mode mode) {
        return *this;
    }

    drawable_base::factory& drawable::factory::col(const buffer& data, size_t offset, size_t stride, blend_mode mode) {
        GLuint index = (GLuint)attributes.size();
        std::stringstream buf;
        buf << "col_attrib_bound_" << index;
        std::string name = buf.str();
        buf << "_var";
        std::string var_name = buf.str();

        attrib attr;
        attr.index = index;
        attr.normalized = GL_FALSE;
        attr.pointer = (GLvoid*)offset;
        attr.size = 4;
        attr.stride = (GLsizei)stride;
        attr.type = GL_FLOAT;

        attributes.emplace(name, std::make_tuple("vec4", attr, data.name));
        varryings.emplace(var_name, std::make_tuple("vec4", name));

        if(outColor.empty()) {
            first_blend_mode = mode;
            outColor = var_name;
        } else {
            std::stringstream color;
            color << "porterduff(" << var_name << ", " << outColor << ", " << blending_name(mode) << ")";
            outColor = color.str();
        }

        return *this;
    }

    drawable_base::factory& drawable::factory::tex(const texture& t, const buffer& b, size_t offset, size_t stride, blend_mode mode) {
        GLuint index = (GLuint)uniforms_texture.size();
        std::stringstream buf;
        buf << "tex_" << index;
        std::string name = buf.str();
        buf << "_coords";
        std::string coords_name = buf.str();
        buf << "_var";
        std::string var_name = buf.str();

        attrib attr;
        attr.index = attributes.size();
        attr.normalized = GL_FALSE;
        attr.pointer = (GLvoid*)offset;
        attr.size = 2;
        attr.stride = (GLsizei)stride;
        attr.type = GL_FLOAT;

        attributes.emplace(coords_name, std::make_tuple("vec2", attr, b.name));
        varryings.emplace(var_name, std::make_tuple("vec2", coords_name));
        uniforms_texture.emplace(name, std::make_tuple(-1, index, t.name));

        std::stringstream color;
        color << "texture2D(" << name << ", " << var_name << ")";

        if(outColor.empty()) {
            first_blend_mode = mode;
            outColor = color.str();
        } else {
            std::stringstream mixed_color;
            mixed_color << "porterduff(" << color.str() << ", " << outColor << ", " << blending_name(mode) << ")";
            outColor = mixed_color.str();
        }

        return *this;
    }

    drawable_base::factory& drawable::factory::tex(const texture&, const float*, blend_mode) {
        return *this;
    }

    drawable_base::factory& drawable::factory::gradient_linear(float angle, uint32_t from, uint32_t to, blend_mode) {
        return *this;
    }

    GLuint create_shader(const char *source, GLenum shader_type) {
        GLuint result = glCreateShader(shader_type);
        assert(result != 0);

        glShaderSource(result, 1, &source, NULL);
        glCompileShader(result);

        GLint status;
        glGetShaderiv(result, GL_COMPILE_STATUS, &status);
        if (!status) {
            char log[1000];
            GLsizei len;
            glGetShaderInfoLog(result, 1000, &len, log);
            std::stringstream error;
            error << "Error compiling " <<
                    (shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment")
                    << " shader: " << log << "\n" << source;
            throw std::runtime_error(error.str());
        }

        return result;
    }

    GLuint drawable::factory::compile_vertex_shader() {
        std::stringstream vertex_source;

        for(auto uniform : uniforms1f)
            vertex_source << "uniform float " << uniform.first << ";\n";
        for(auto uniform : uniforms4f)
            vertex_source << "uniform vec4 " << uniform.first << ";\n";
        for(auto uniform : uniformsMatrix4fv)
            vertex_source << "uniform mat4 " << uniform.first << ";\n";
        for(auto uniform : uniforms_texture)
            vertex_source << "uniform sampler2D " << uniform.first << ";\n";

        for(auto attr : attributes)
            vertex_source << "attribute " << std::get<0>(attr.second) << " " << attr.first << ";\n";

        for(auto var : varryings)
            vertex_source << "varying " << std::get<0>(var.second) << " " << var.first << ";\n";

        vertex_source << "void main() {\n"
                "\tgl_Position = " << outPos << ";\n";

        for(auto var : varryings)
            vertex_source << "\t" << var.first << " = " << std::get<1>(var.second) << ";\n";

        vertex_source << "}\n";

        std::cout << vertex_source.str() << std::endl;

        return create_shader(vertex_source.str().c_str(), GL_VERTEX_SHADER);
    }

    GLuint drawable::factory::compile_fragment_shader() {
        std::stringstream frag_source;

        frag_source << "precision mediump float;\n";

        for(auto uniform : uniforms1f)
            frag_source << "uniform float " << uniform.first << ";\n";
        for(auto uniform : uniforms4f)
            frag_source << "uniform vec4 " << uniform.first << ";\n";
        for(auto uniform : uniformsMatrix4fv)
            frag_source << "uniform mat4 " << uniform.first << ";\n";
        for(auto uniform : uniforms_texture)
            frag_source << "uniform sampler2D " << uniform.first << ";\n";

        for(auto var : varryings) frag_source << "varying " << std::get<0>(var.second) << " " << var.first << ";\n";

        frag_source <<
                "vec4 blend_opacity(in vec4 col, in float opacity) {\n"
                    "\treturn vec4(col.xyz, col.w * opacity);\n"
                "}\n"
                "const int BLEND_NONE = -1;\n"
                "const int BLEND_CLEAR = 0;\n"
                "const int BLEND_SRC = 1;\n"
                "const int BLEND_DST = 2;\n"
                "const int BLEND_XOR = 3;\n"
                "const int BLEND_ATOP = 4;\n"
                "const int BLEND_OVER = 5;\n"
                "const int BLEND_IN = 6;\n"
                "const int BLEND_OUT = 7;\n"
                "const int BLEND_DST_ATOP = 8;\n"
                "const int BLEND_DST_OVER = 9;\n"
                "const int BLEND_DST_IN = 10;\n"
                "const int BLEND_DST_OUT = 11;\n"

                "vec4 porterduff(in vec4 dest, in vec4 source, int mode) {\n"
                    // http://cairographics.org/operators/

                    "\tif(mode == BLEND_CLEAR) return vec4(0);\n"
                    "\telse if(mode == BLEND_SRC) return source;\n"
                    "\telse if(mode == BLEND_DST) return dest;\n"

                    "\telse if(mode == BLEND_XOR) {\n"
                        "\t\tfloat aR = source.a + dest.a - 2.f * (source.a * dest.a);\n"
                        "\t\tvec3 xaA = source.rgb * source.a;\n"
                        "\t\tvec3 xaB = dest.rgb * dest.a;\n"
                        "\t\tvec3 xR = (xaA * (1.f - dest.a) + xaB * (1.f - source.a)) / aR;\n"
                        "\t\treturn vec4(xR, aR);\n"
                    "\t}\n"

                    "\telse if(mode == BLEND_OVER || mode == BLEND_NONE) {\n"
                        "\t\tfloat aR = source.a + dest.a * (1.f - source.a);\n"
                        "\t\tvec3 xaA = source.rgb * source.a;\n"
                        "\t\tvec3 xaB = dest.rgb * dest.a;\n"
                        "\t\tvec3 xR = (xaA + xaB * (1.f - source.a)) / aR;\n"
                        "\t\treturn vec4(xR, aR);\n"
                    "\t}\n"

                    "\telse if(mode == BLEND_ATOP) {\n"
                        "\t\tfloat aR = dest.a;\n"
                        "\t\tvec3 xaA = source.rgb * source.a;\n"
                        "\t\tvec3 xR = xaA + dest.rgb * (1.f - source.a);\n"
                        "\t\treturn vec4(xR, aR);\n"
                    "\t}\n"

                    "\telse if(mode == BLEND_IN) return source;\n" //TODO
                    "\telse if(mode == BLEND_OUT) return source;\n"

                    "\telse if(mode == BLEND_DST_ATOP) return source;\n"
                    "\telse if(mode == BLEND_DST_OVER) return source;\n"
                    "\telse if(mode == BLEND_DST_IN) return source;\n"
                    "\telse if(mode == BLEND_DST_OUT) return source;\n"

                    "\treturn mix(dest, source, 0.5);\n"
                "}\n"
                "void main() {\n"
                    "\tgl_FragColor = " << outColor << ";\n"
                "}\n";

        std::cout << frag_source.str() << std::endl;

        return create_shader(frag_source.str().c_str(), GL_FRAGMENT_SHADER);
    }

    drawable drawable::factory::compile(vertices_primitive_mode mode, const buffer& buf, size_t offset, size_t count) {
        drawable result = compile(mode, count);

        result.indices_buffer = buf.name;
        result.indices_offset = offset;

        return result;
    }

    drawable drawable::factory::compile(vertices_primitive_mode mode, size_t count) {
        drawable result;

        runtime_assert(outPos != "", "No position defined for drawable");

        result.has_blend = true;
        switch(first_blend_mode) {
            // http://www.andersriggelsen.dk/glblendfunc.php
            case BLEND_NONE:        result.has_blend = false; break;

            case BLEND_CLEAR:       result.blend_src = GL_ZERO;
                                    result.blend_dst = GL_ZERO; break;

            case BLEND_SRC:         result.blend_src = GL_ONE;
                                    result.blend_dst = GL_ZERO; break;

            case BLEND_DST:         result.blend_src = GL_ZERO;
                                    result.blend_dst = GL_ONE; break;

            case BLEND_XOR:         result.blend_src = GL_ONE_MINUS_DST_ALPHA;
                                    result.blend_dst = GL_ONE_MINUS_SRC_ALPHA; break;

            case BLEND_ATOP:        result.blend_src = GL_DST_ALPHA;
                                    result.blend_dst = GL_ONE_MINUS_SRC_ALPHA; break;

            case BLEND_OVER:        result.blend_src = GL_SRC_ALPHA;
                                    result.blend_dst = GL_ONE_MINUS_SRC_ALPHA; break;

            case BLEND_IN:          result.blend_src = GL_DST_ALPHA;
                                    result.blend_dst = GL_ZERO; break;

            case BLEND_OUT:         result.blend_src = GL_ONE_MINUS_DST_ALPHA;
                                    result.blend_dst = GL_ZERO; break;

            case BLEND_DST_ATOP:    result.blend_src = GL_ONE_MINUS_DST_ALPHA;
                                    result.blend_dst = GL_SRC_ALPHA; break;

            case BLEND_DST_OVER:    result.blend_src = GL_ONE_MINUS_DST_ALPHA;
                                    result.blend_dst = GL_ONE; break;

            case BLEND_DST_IN:      result.blend_src = GL_ZERO;
                                    result.blend_dst = GL_SRC_ALPHA; break;

            case BLEND_DST_OUT:     result.blend_src = GL_ZERO;
                                    result.blend_dst = GL_ONE_MINUS_SRC_ALPHA; break;
        }

        switch(mode) {
            case PRIMITIVE_POINTS: result.primitive_mode = GL_POINTS; break;
            case PRIMITIVE_LINE_STRIP: result.primitive_mode = GL_LINE_STRIP; break;
            case PRIMITIVE_LINE_LOOP: result.primitive_mode = GL_LINE_LOOP; break;
            case PRIMITIVE_LINES: result.primitive_mode = GL_LINES; break;
            case PRIMITIVE_TRIANGLE_STRIP: result.primitive_mode = GL_TRIANGLE_STRIP; break;
            case PRIMITIVE_TRIANGLE_FAN: result.primitive_mode = GL_TRIANGLE_FAN; break;
            case PRIMITIVE_TRIANGLES: result.primitive_mode = GL_TRIANGLES; break;
        }
        result.primitive_count = count;

        GLuint vertex_shader = compile_vertex_shader();
        GLuint frag_shader = compile_fragment_shader();
        result.name = glCreateProgram();
        glAttachShader(result.name, vertex_shader);
        glAttachShader(result.name, frag_shader);

        for (auto attr : attributes) {
            glBindAttribLocation(result.name, std::get<1>(attr.second).index, attr.first.c_str());
            result.attributes.emplace_back(std::get<1>(attr.second), std::get<2>(attr.second));
        }

        glLinkProgram(result.name);

        GLint status;
        glGetProgramiv(result.name, GL_LINK_STATUS, &status);
        if (!status) {
            char log[1000];
            GLsizei len;
            glGetProgramInfoLog(result.name, 1000, &len, log);
            std::stringstream error;
            error << "Error linking drawable: " << log;
            throw std::runtime_error(error.str());
        }

        for (auto uniform : uniforms1f) {
            std::get<0>(uniform.second) = glGetUniformLocation(result.name, uniform.first.c_str());
            if(std::get<0>(uniform.second) != -1)
                result.uniforms1f.emplace_back(uniform.second);
        }

        for(auto uniform : uniforms4f) {
            std::get<0>(uniform.second) = glGetUniformLocation(result.name, uniform.first.c_str());
            if(std::get<0>(uniform.second) != -1)
                result.uniforms4f.emplace_back(uniform.second);
        }

        for(auto uniform : uniformsMatrix4fv) {
            std::get<0>(uniform.second) = glGetUniformLocation(result.name, uniform.first.c_str());
            if(std::get<0>(uniform.second) != -1)
                result.uniformsMatrix4fv.emplace_back(uniform.second);
        }

        for(auto uniform : uniforms_texture) {
            std::get<0>(uniform.second) = glGetUniformLocation(result.name, uniform.first.c_str());
            if(std::get<0>(uniform.second) != -1)
                result.uniforms_texture.emplace_back(uniform.second);
        }

        assert(glGetError() == GL_NO_ERROR);

        return result;
    }

    void drawable::draw() const {
        glUseProgram(name);

        for (auto attribute : attributes) {
            attrib attr = std::get<0>(attribute);
            glEnableVertexAttribArray(attr.index);
            glBindBuffer(GL_ARRAY_BUFFER, std::get<1>(attribute));
            glVertexAttribPointer(attr.index, attr.size, attr.type, attr.normalized, attr.stride, attr.pointer);
        }

        for (auto uniform : uniforms1f) {
            glUniform1f(std::get<0>(uniform), *std::get<1>(uniform));
        }

        for (auto uniform : uniforms4f) {
            const vec4 &uni = *std::get<1>(uniform);
            glUniform4f(std::get<0>(uniform), uni.r, uni.g, uni.b, uni.a);
        }

        for (auto uniform : uniformsMatrix4fv)
            glUniformMatrix4fv(std::get<0>(uniform), 1, GL_FALSE, std::get<1>(uniform));

        for (auto uniform : uniforms_texture) {
            glActiveTexture(GL_TEXTURE0 + std::get<1>(uniform));
            glBindTexture(GL_TEXTURE_2D, std::get<2>(uniform));
            glUniform1i(std::get<0>(uniform), std::get<1>(uniform));
        }

        if (has_blend) {
            glEnable(GL_BLEND);
            glBlendFunc(blend_src, blend_dst);
        }

        if (indices_buffer != GL_NONE) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer);
            glDrawElements(primitive_mode, primitive_count, GL_UNSIGNED_SHORT, (void *) indices_offset);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_NONE);
        } else {
            glDrawArrays(primitive_mode, 0, primitive_count);
        }

        if (has_blend) {
            glDisable(GL_BLEND);
        }

        for (auto attr : attributes)
            glDisableVertexAttribArray(std::get<0>(attr).index);
    }

} // namespace hut
