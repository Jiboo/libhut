#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
  mat4 proj;
  mat4 view;
  float dpi_factor;
} ubo;

layout(location = 0) in uvec4 in_i_pos_box_r16g16b16a16_uint; // AA box (x1, y1, x2, y2), 4 MSB for each component used for flags
layout(location = 1) in vec4 in_i_uv_box_r16g16b16a16_unorm; // AA box (x1, y1, x2, y2)
layout(location = 2) in vec4 in_i_col_from_r8g8b8a8_unorm;
layout(location = 3) in vec4 in_i_col_to_r8g8b8a8_unorm;

layout(location = 0) out flat vec4 out_box;
layout(location = 1) out flat uvec4 out_params; // (radius, softness, atlas_page, gradient)
layout(location = 2) out vec4 out_col;
layout(location = 3) out vec2 out_uv;

out gl_PerVertex {
  vec4 gl_Position;
};

/*
  4 MSB of pos.x: round corner radius (4px per step)
  4 MSB of pos.y: round corner softness (2px per step)
  4 MSB of pos.z: atlas page
  4 MSB of pos.w: gradient
*/

const uint GRADIENT_T2B = 0;
const uint GRADIENT_L2R = 1;
const uint GRADIENT_TL2BR = 2;
const uint GRADIENT_TR2BL = 3;
const uint INVALID_ATLAS_PAGE = 0xFFFFFFFF;

void main() {
  const vec4 uv_box = in_i_uv_box_r16g16b16a16_unorm;

  const vec4 col_from = in_i_col_from_r8g8b8a8_unorm;
  const vec4 col_to = in_i_col_to_r8g8b8a8_unorm;
  const vec4 col_mix = mix(col_from, col_to, 0.5f);

  const float radius = (in_i_pos_box_r16g16b16a16_uint.x >> 12) * 4 * ubo.dpi_factor;
  const float softness = (in_i_pos_box_r16g16b16a16_uint.y >> 12) * ubo.dpi_factor;
  const float softness2 = softness * 2;

  out_params.x = floatBitsToUint(radius);
  out_params.y = floatBitsToUint(softness);
  out_params.z = uv_box == vec4(0, 0, 0, 0) ? INVALID_ATLAS_PAGE : (in_i_pos_box_r16g16b16a16_uint.z >> 12);
  out_params.w = in_i_pos_box_r16g16b16a16_uint.w >> 12;

  const uvec4 pos_box_raw = in_i_pos_box_r16g16b16a16_uint & 0x0FFF;
  const vec4 pos_box_tl = ubo.view * vec4(pos_box_raw.xy, 0, 1);
  const vec4 pos_box_br = ubo.view * vec4(pos_box_raw.zw, 0, 1);
  const vec4 pos_box = vec4(pos_box_tl.xy, pos_box_br.xy);
  out_box = vec4(pos_box.x + softness2, pos_box.y + softness2, pos_box.z - softness2, pos_box.w - softness2);

  const uint indices[] = {0, 1, 2, 2, 1, 3};

  switch (indices[gl_VertexIndex]) {
    case 0: // top left
      gl_Position = ubo.proj * vec4(pos_box.x, pos_box.y, 0, 1);
      out_uv = vec2(uv_box.x, uv_box.y);
      out_col = out_params.w == GRADIENT_TR2BL ? col_mix : col_from;
    break;
    case 1: // bottom left
      gl_Position = ubo.proj * vec4(pos_box.x, pos_box.w, 0, 1);
      out_uv = vec2(uv_box.x, uv_box.w);
      if (out_params.w == GRADIENT_T2B) out_col = col_to;
      else if (out_params.w == GRADIENT_TL2BR) out_col = col_mix;
      else if (out_params.w == GRADIENT_TR2BL) out_col = col_to;
      else out_col = col_from;
    break;
    case 2: // top right
      gl_Position = ubo.proj * vec4(pos_box.z, pos_box.y, 0, 1);
      out_uv = vec2(uv_box.z, uv_box.y);
      if (out_params.w == GRADIENT_L2R) out_col = col_to;
      else if (out_params.w == GRADIENT_TL2BR) out_col = col_mix;
      else out_col = col_from;
    break;
    case 3: // bottom right
      gl_Position = ubo.proj * vec4(pos_box.z, pos_box.w, 0, 1);
      out_uv = vec2(uv_box.z, uv_box.w);
      out_col = out_params.w == GRADIENT_TR2BL ? col_mix : col_to;
    break;
  }
}
