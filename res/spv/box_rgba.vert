#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
} ubo;

layout(location = 0) in vec2 in_v_pos;
layout(location = 1) in vec4 in_v_col;

layout(location = 2) in mat4 in_i_transform;
layout(location = 6) in vec2 in_i_params; //  corner radius, blur radius
layout(location = 7) in vec4 in_i_bounds;

layout(location = 0) out vec4 out_col;
layout(location = 1) out vec2 out_params;
layout(location = 2) out vec4 out_bounds;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    float blur = in_i_params[1];
    vec2 pos = in_v_pos.xy;

    switch(gl_VertexIndex) {
        case 0: // top left
            pos = pos - blur;
            break;
        case 1: // bot left
            pos.x = pos.x - blur;
            pos.y = pos.y + blur;
            break;
        case 2: // top right
            pos.x = pos.x + blur;
            pos.y = pos.y - blur;
            break;
        case 3: // bot right
            pos = pos + blur;
            break;
    }

    out_col = in_v_col;
    out_params = in_i_params;
    out_bounds = in_i_bounds;

    gl_Position = ubo.proj * in_i_transform * vec4(pos, 0.0, 1.0);
}
