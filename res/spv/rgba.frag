#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 in_col;

layout(location = 0) out vec4 out_col;

void main() {
    out_col = in_col;
}
