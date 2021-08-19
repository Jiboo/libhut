#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_col;

layout(location = 0) out vec4 out_col;

void main() {
    vec4 texColor = texture(texSampler, in_uv);
    out_col = mix(in_col, texColor, texColor.a * in_col.a);
}
