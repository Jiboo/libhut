#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 1) uniform sampler2D uni_samplers[4];

layout(location = 0) in vec4 in_col;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_col;

void main() {
  uint atlas_page = 0;
  if (in_uv.x > 0 && in_uv.y < 0)
      atlas_page = 1;
  else if (in_uv.x < 0 && in_uv.y > 0)
      atlas_page = 2;
  else if (in_uv.x < 0 && in_uv.y < 0)
      atlas_page = 3;

  const vec2 uv = abs(in_uv);
  const vec4 tex = texture(uni_samplers[nonuniformEXT(atlas_page)], uv);

  out_col = in_col;
  out_col.a *= tex.r;
}
