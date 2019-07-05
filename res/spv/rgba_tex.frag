#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, inTexCoord);
    outColor = mix(inColor, texColor, texColor.a);
}
