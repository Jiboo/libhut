#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
    float dpi_scale;
} ubo;

layout(location = 0) in vec2 in_v_pos;
layout(location = 1) in vec3 in_v_col;

layout(location = 2) in mat4 in_i_transform;

layout(location = 0) out vec3 out_col;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.proj * in_i_transform * vec4(in_v_pos, 0.0, 1.0);
    out_col = in_v_col;
}
