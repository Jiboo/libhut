#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D setSampler;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(setSampler, inTexCoord);
    outColor = inColor;
    outColor.a *= texColor.r;
}