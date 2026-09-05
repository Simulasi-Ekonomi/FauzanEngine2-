#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 instanceM0;
layout(location = 4) in vec4 instanceM1;
layout(location = 5) in vec4 instanceM2;
layout(location = 6) in vec4 instanceM3;

layout(push_constant) uniform Transform {
    mat4 mvp;
} transform;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV;

void main() {
    mat4 instanceTransform = mat4(instanceM0, instanceM1, instanceM2, instanceM3);
    gl_Position = transform.mvp * instanceTransform * vec4(inPosition, 1.0);
    outNormal = inNormal;
    outUV = inUV;
}
