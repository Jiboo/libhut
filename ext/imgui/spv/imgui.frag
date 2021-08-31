#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D uni_sampler;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_col;

layout(location = 0) out vec4 out_col;

void main() {
    vec4 tex_col = texture(uni_sampler, in_uv);
    out_col = in_col * tex_col;
}
