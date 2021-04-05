#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform samplerCube cube_sampler;

layout(location = 0) in vec3 in_uvw;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(cube_sampler, in_uvw);
}
