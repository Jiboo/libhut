#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
    mat4 view;
} ubo;

layout(location = 0) in vec3 inVertexPosition;
layout(location = 1) in vec3 inVertexColor;

layout(location = 2) in mat4 inInstanceTransform;

layout(location = 0) out vec3 outColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.proj * ubo.view * inInstanceTransform * vec4(inVertexPosition, 1.0);
    outColor = inVertexColor;
}
