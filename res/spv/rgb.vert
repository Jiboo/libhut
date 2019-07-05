#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inVertexPosition;
layout(location = 1) in vec3 inVertexColor;

layout(location = 2) in vec4 inInstanceCol0;
layout(location = 3) in vec4 inInstanceCol1;
layout(location = 4) in vec4 inInstanceCol2;
layout(location = 5) in vec4 inInstanceCol3;

layout(location = 0) out vec3 outColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    mat4 transform = mat4(inInstanceCol0, inInstanceCol1, inInstanceCol2, inInstanceCol3);
    gl_Position = ubo.proj * transform * vec4(inVertexPosition, 0.0, 1.0);
    outColor = inVertexColor;
}
