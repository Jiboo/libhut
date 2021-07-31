#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D uni_sampler;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_col;

layout(location = 0) out vec4 out_col;

void main() {
    vec4 texColor = texture(uni_sampler, in_uv);
    out_col = mix(vec4(in_col, 1.0), texColor, texColor.a);
}
