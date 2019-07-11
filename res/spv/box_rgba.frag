#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec4 inParams; // offset x/y, corner radius, shadow radius
layout(location = 2) in vec4 inShadowColor;
layout(location = 3) in vec4 inBoxBounds;

layout(location = 0) out vec4 outColor;

// credits: https://www.shadertoy.com/view/llffD8
float rrect(in vec2 uv, in vec2 pos, in vec2 size, float rad, float s)
{
    // calculate new origin and size to account for the space taken by the radius
    vec2 sz = size - vec2(rad, rad) * 2.0;
    vec2 p = pos + vec2(rad, rad);
    vec2 end = p + sz;

    float dx = max(max(uv.x - end.x, 0.0), max(p.x - uv.x, 0.0));
    float dy = max(max(uv.y - end.y, 0.0), max(p.y - uv.y, 0.0));

    float d = sqrt(dx * dx + dy * dy);  // distance from the inner sharp rectangle

    /* by centering the 1-smootstep zero-crossing at "rad" we define our shape as
	 * the locus of all points with distance < "rad" from the sharp rect. This naturally
	 * produces rounded corners, since the points equidistant (at distance "rad") from
	 * the corners form arcs.
	 */
    return 1.0 - smoothstep(rad - s, rad + s, d);
}
void draw_rect(inout vec4 pixel, in vec2 uv, in vec2 pos, in vec2 size, in vec2 offset, float corner_rad, float shadow_blur)
{
    pixel = mix(pixel, inShadowColor, inShadowColor.a * rrect(uv, pos + offset, size, corner_rad, shadow_blur));
    pixel = mix(pixel, inColor,       rrect(uv, pos, size, corner_rad, 0.001));
}

void main() {
    float blur = inParams[3];
    vec2 box_screen_offset = inBoxBounds.xy;
    vec2 box_screen_size = vec2(inBoxBounds[2] - inBoxBounds[0], inBoxBounds[3] - inBoxBounds[1]);
    vec2 frag_bounds_offset = min(box_screen_offset + inParams.xy, box_screen_offset) - blur;
    vec2 frag_bounds_size = box_screen_size + abs(inParams.xy) + 2 * blur;
    vec2 screen = gl_FragCoord.xy;
    float aspect = frag_bounds_size.x / frag_bounds_size.y;
    vec2 screen_pos = screen - frag_bounds_offset;
    vec2 uv = vec2(aspect, 1.0) * screen_pos.xy / frag_bounds_size;

    vec2 shadow_offset = vec2(aspect, 1.0) * inParams.xy / frag_bounds_size;
    vec2 params_radius = vec2(aspect, 1.0) * inParams.zw / frag_bounds_size;
    vec2 box_uv_offset = vec2(aspect, 1.0) * (box_screen_offset - frag_bounds_offset) / frag_bounds_size;
    vec2 box_uv_size = vec2(aspect, 1.0) * box_screen_size / frag_bounds_size;

    vec4 col = vec4(0, 0, 0, 0);
    draw_rect(col, uv, box_uv_offset, box_uv_size, shadow_offset, params_radius.x, params_radius.y);
    outColor = col;
}
