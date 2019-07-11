#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inVertexPosition;
layout(location = 1) in vec4 inVertexColor;

layout(location = 2) in vec4 inInstanceCol0;
layout(location = 3) in vec4 inInstanceCol1;
layout(location = 4) in vec4 inInstanceCol2;
layout(location = 5) in vec4 inInstanceCol3;
layout(location = 6) in vec4 inInstanceParams; // offset x/y, corner radius, shadow radius
layout(location = 7) in vec4 inInstanceShadowColor;
layout(location = 8) in vec4 inInstanceBoxBounds;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outParams;
layout(location = 2) out vec4 outShadowColor;
layout(location = 3) out vec4 outBoxBounds;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    mat4 transform = mat4(inInstanceCol0, inInstanceCol1, inInstanceCol2, inInstanceCol3);

    float blur = inInstanceParams[3];
    vec2 pos = inVertexPosition.xy;

    switch(gl_VertexIndex) {
        case 0: // top left
            pos = min(pos + inInstanceParams.xy, pos) - blur;
            break;
        case 1: // top right
            pos.x = max(pos.x + inInstanceParams.x, pos.x) + blur;
            pos.y = min(pos.y + inInstanceParams.y, pos.y) - blur;
            break;
        case 2: // bot right
            pos = max(pos + inInstanceParams.xy, pos) + blur;
            break;
        case 3: // bot left
            pos.x = min(pos.x + inInstanceParams.x, pos.x) - blur;
            pos.y = max(pos.y + inInstanceParams.y, pos.y) + blur;
    }

    outColor = inVertexColor;
    outParams = inInstanceParams;
    outShadowColor = inInstanceShadowColor;
    outBoxBounds = inInstanceBoxBounds;

    gl_Position = ubo.proj * transform * vec4(pos, 0.0, 1.0);
}
