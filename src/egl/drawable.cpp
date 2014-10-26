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
            case BNONE: return "BNONE";
            case BCLEAR: return "BCLEAR";
            case BSOURCE: return "BSOURCE";
            case BDEST: return "BDEST";
            case BXOR: return "BXOR";
            case BATOP: return "BATOP";
            case BOVER: return "BOVER";
            case BIN: return "BIN";
            case BOUT: return "BOUT";
            case BDEST_ATOP: return "BDEST_ATOP";
            case BDEST_OVER: return "BDEST_OVER";
            case BDEST_IN: return "BDEST_IN";
            case BDEST_OUT: return "BDEST_OUT";
        }
        return "BNONE";
    }

    drawable::factory& drawable::factory::pos(const buffer& data, size_t offset, size_t stride) {
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

    drawable::factory& drawable::factory::transform(const mat4& ref) {
        runtime_assert(outPos != "", "Define position before transforms");

        std::stringstream name;
        name << "transform_" << uniformsMatrix4fv.size();

        uniformsMatrix4fv.emplace(name.str(), std::make_tuple(-1, ref.data->data));
        outPos += '*' + name.str();
        return *this;
    }

    drawable::factory& drawable::factory::opacity(const float& ref) {
        std::stringstream buf;
        buf << "opacity_" << uniforms1f.size();
        std::string name = buf.str();

        uniforms1f.emplace(name, std::make_tuple(-1, &ref));

        std::stringstream color;
        color << "blend_opacity(" << outColor << ", " << name << ")";
        outColor = color.str();

        return *this;
    }

    drawable::factory& drawable::factory::col(const vec4& ref, blend_mode mode) {
        std::stringstream buf;
        buf << "col_uni_" << uniforms4f.size();
        std::string name = buf.str();

        uniforms4f.emplace(name, std::make_tuple(-1, &ref));

        if(outColor.empty()) {
            first_mode = mode;
            outColor = name;
        } else {
            std::stringstream color;
            color << "porterduff(" << name << ", " << outColor << ", " << blending_name(mode) << ")";
            outColor = color.str();
        }

        return *this;
    }

    drawable::factory& drawable::factory::col(const vec4* colors, blend_mode mode) {
        return *this;
    }

    drawable::factory& drawable::factory::col(const buffer& data, size_t offset, size_t stride, blend_mode mode) {
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
            first_mode = mode;
            outColor = var_name;
        } else {
            std::stringstream color;
            color << "porterduff(" << var_name << ", " << outColor << ", " << blending_name(mode) << ")";
            outColor = color.str();
        }

        return *this;
    }

    drawable::factory& drawable::factory::tex(const texture&, const buffer&, size_t offset, size_t stride, blend_mode) {
        return *this;
    }

    drawable::factory& drawable::factory::tex(const texture&, const float*, blend_mode) {
        return *this;
    }

    drawable::factory& drawable::factory::gradient_linear(float angle, uint32_t from, uint32_t to, blend_mode) {
        return *this;
    }

    GLuint create_shader(const char *source, GLenum shader_type) {
        GLuint result = glCreateShader(shader_type);
        assert(result != 0);

        std::cout << source;

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

        for(auto attr : attributes)
            vertex_source << "attribute " << std::get<0>(attr.second) << " " << attr.first << ";\n";

        for(auto var : varryings)
            vertex_source << "varying " << std::get<0>(var.second) << " " << var.first << ";\n";

        vertex_source << "void main() {\n"
                "\tgl_Position = " << outPos << ";\n";

        for(auto var : varryings)
            vertex_source << "\t" << var.first << " = " << std::get<1>(var.second) << ";\n";

        vertex_source << "}\n";

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

        for(auto var : varryings) frag_source << "varying " << std::get<0>(var.second) << " " << var.first << ";\n";

        frag_source <<
                "vec4 blend_opacity(in vec4 col, in float opacity) {\n"
                "\treturn vec4(col.xyz, col.w * opacity);\n"
                "}\n"
                "const int BOVER = 5;\n"
                "vec4 porterduff(in vec4 dest, in vec4 source, int mode) {\n"
                "\treturn mix(dest, source, 0.5);\n"
                "}\n"
                "void main() {\n"
                "\tgl_FragColor = " << outColor << ";\n"
                "}\n";

        return create_shader(frag_source.str().c_str(), GL_FRAGMENT_SHADER);
    }

    drawable drawable::factory::compile() {
        drawable result;

        runtime_assert(outPos != "", "No position defined for drawable");

        runtime_assert(first_mode != BNONE, "drawable don't have any color/texture input");
        result.first_mode = first_mode;

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
            std::get<0>(uniform.second) = glGetUniformLocation(result.name, uniform.first.c_str());;
            result.uniforms1f.emplace_back(uniform.second);
        }

        for(auto uniform : uniforms4f) {
            std::get<0>(uniform.second) = glGetUniformLocation(result.name, uniform.first.c_str());
            result.uniforms4f.emplace_back(uniform.second);
        }

        for(auto uniform : uniformsMatrix4fv) {
            std::get<0>(uniform.second) = glGetUniformLocation(result.name, uniform.first.c_str());
            result.uniformsMatrix4fv.emplace_back(uniform.second);
        }

        assert(glGetError() == GL_NO_ERROR);

        return result;
    }

    void drawable::bind() {
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

        for(auto uniform : uniformsMatrix4fv)
            glUniformMatrix4fv(std::get<0>(uniform), 1, GL_FALSE, std::get<1>(uniform));
    }

    void drawable::unbind() {
        for(auto attr : attributes)
            glDisableVertexAttribArray(std::get<0>(attr).index);
    }

} // namespace hut
