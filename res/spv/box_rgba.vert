#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inVertexPosition;
layout(location = 1) in vec4 inVertexColor;

layout(location = 2) in mat4 inInstanceTransform;
layout(location = 6) in vec2 inInstanceParams; //  corner radius, blur radius
layout(location = 7) in vec4 inInstanceBoxBounds;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outParams;
layout(location = 2) out vec4 outBoxBounds;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    float blur = inInstanceParams[1];
    vec2 pos = inVertexPosition.xy;

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

    outColor = inVertexColor;
    outParams = inInstanceParams;
    outBoxBounds = inInstanceBoxBounds;

    gl_Position = ubo.proj * inInstanceTransform * vec4(pos, 0.0, 1.0);
}
