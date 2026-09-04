#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(push_constant) uniform Transform {
    mat4 mvp;
} transform;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV;

void main() {
    gl_Position = transform.mvp * vec4(inPosition, 1.0);
    outNormal = inNormal;
    outUV = inUV;
}
