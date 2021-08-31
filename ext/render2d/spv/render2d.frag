#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 1) uniform sampler2D uni_samplers[4];

layout(location = 0) in flat vec4 in_box; // pos_box
layout(location = 1) in flat uvec4 in_params; // (radius, smoothness, atlas_page, gradient)
layout(location = 2) in vec4 in_col;
layout(location = 3) in vec2 in_uv;

in vec4 gl_FragCoord;

layout(location = 0) out vec4 out_col;

float udRoundBox(vec2 p, vec2 b, float r) {
  return length(max(abs(p) - b + r, 0.f)) - r;
}

float box_effects(float _radius, float _softness) {
  vec2 top_left = vec2(in_box.x, in_box.y);
  vec2 bot_right = vec2(in_box.z, in_box.w);
  vec2 half_size = (bot_right - top_left) / 2;
  vec2 center = half_size + top_left;
  float distance = udRoundBox(gl_FragCoord.xy - center, half_size, _radius);
  return 1.0f - smoothstep(0.0f, _softness * 2, distance);
}

void main() {
  uint atlas_page = in_params.z;
  out_col = in_col;
  if (atlas_page < 4)
      out_col *= texture(uni_samplers[nonuniformEXT(atlas_page)], in_uv);
  const float radius = uintBitsToFloat(in_params.x);
  const float softness = uintBitsToFloat(in_params.y);
  if (radius > 0 || softness > 0)
    out_col.a *= box_effects(radius, softness);
}
