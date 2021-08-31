#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
  mat4 proj;
  mat4 view;
  float dpi_factor;
} ubo;

layout(location = 0) in vec2 in_v_pos;
layout(location = 1) in vec2 in_v_uv;
layout(location = 2) in vec4 in_v_col_r8g8b8a8_unorm;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_col;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.proj * ubo.view * vec4(in_v_pos, 0.0, 1.0);
    out_col = in_v_col_r8g8b8a8_unorm;
    out_uv = in_v_uv;
}
