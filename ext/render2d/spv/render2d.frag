#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

const int samplers_pages = 16;
layout(binding = 1) uniform sampler2D uni_samplers[samplers_pages];

layout(location = 0) in flat vec4 in_box; // pos_box
layout(location = 1) in flat uvec4 in_params; // (radius, smoothness, atlas_page, mode)
layout(location = 2) in vec4 in_col;
layout(location = 3) in vec2 in_uv;

in vec4 gl_FragCoord;

layout(location = 0) out vec4 out_col;

const uint RENDER_GRADIENT = 0;
const uint RENDER_BORDER = 1;
const uint RENDER_SHADOW = 2;

float udRoundBox(vec2 _center, vec2 _half_size, float _radius) {
  return length(max(abs(_center) - _half_size + _radius, 0.f)) - _radius;
}

float border_distance(float _radius) {
  vec2 top_left = vec2(in_box.x, in_box.y);
  vec2 bot_right = vec2(in_box.z, in_box.w);
  vec2 half_size = (bot_right - top_left) / 2;
  vec2 center = top_left + half_size;
  return udRoundBox(gl_FragCoord.xy - center, half_size, _radius);
}

void main() {
  const uint atlas_page = in_params.z;
  const vec4 tex_sample = (atlas_page < samplers_pages)
    ? texture(uni_samplers[nonuniformEXT(atlas_page)], in_uv)
    : vec4(1);
  const float radius = uintBitsToFloat(in_params.x);
  const float softness = uintBitsToFloat(in_params.y);
  const float smoothness = 1;

  out_col = in_col * tex_sample;
  switch(in_params.w) {
    case RENDER_GRADIENT: out_col.a *= 1 - smoothstep(0, softness, border_distance(radius)); break;
    case RENDER_BORDER: out_col.a *= 1 - smoothstep(softness/2 - smoothness, softness/2, abs(border_distance(radius/2))); break;
    case RENDER_SHADOW: out_col.a *= smoothstep(0, softness, border_distance(radius)); break;
    default: out_col = vec4(1,0,0,1);
  }
}
