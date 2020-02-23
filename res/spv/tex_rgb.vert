#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inVertexPosition;
layout(location = 1) in vec2 inVertexTexCoord;
layout(location = 2) in vec3 inVertexColor;

layout(location = 3) in mat4 inInstanceTransform;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.proj * inInstanceTransform * vec4(inVertexPosition, 0.0, 1.0);
    outColor = inVertexColor;
    outTexCoord = inVertexTexCoord;
}
