#version 330 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

layout(location=3) in ivec4 boneIDs;
layout(location=4) in vec4 weights;

uniform mat4 bones[128];
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    mat4 skin =
          weights.x * bones[boneIDs.x]
        + weights.y * bones[boneIDs.y]
        + weights.z * bones[boneIDs.z]
        + weights.w * bones[boneIDs.w];

    vec4 pos = skin * vec4(position,1.0);

    gl_Position = proj * view * model * pos;
}
