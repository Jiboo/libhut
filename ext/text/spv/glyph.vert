#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
  mat4 proj;
  mat4 view;
  float dpi_factor;
} ubo;

layout(location = 0) in ivec2 in_v_pos_r16g16_sint;
layout(location = 1) in vec2 in_v_uv_r16g16_snorm;

layout(location = 2) in uvec2 in_i_translate_r16g16_uint;
layout(location = 3) in vec4 in_i_col_r8g8b8a8_unorm;

layout(location = 0) out vec4 out_col;
layout(location = 1) out vec2 out_uv;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  vec2 pos = vec2(in_v_pos_r16g16_sint) + vec2(in_i_translate_r16g16_uint);
  gl_Position = ubo.proj * ubo.view * vec4(pos, 0.0, 1.0);
  out_col = in_i_col_r8g8b8a8_unorm;
  out_uv = in_v_uv_r16g16_snorm;
}
