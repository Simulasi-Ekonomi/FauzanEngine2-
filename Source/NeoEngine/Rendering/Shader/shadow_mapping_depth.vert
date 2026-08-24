#version 300 es
// Shadow Mapping Depth Vertex Shader - OpenGL ES 3.0
// Renders scene from light's perspective to generate depth map

precision highp float;

layout(location = 0) in vec3 aPos;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

void main()
{
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPos, 1.0);
}
