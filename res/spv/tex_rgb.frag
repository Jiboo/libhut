#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D setSampler;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(setSampler, inTexCoord);
    outColor = mix(vec4(inColor, 1.0), texColor, texColor.a);
}
