#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 instanceM0;
layout(location = 4) in vec4 instanceM1;
layout(location = 5) in vec4 instanceM2;
layout(location = 6) in vec4 instanceM3;
layout(location = 7) in uvec4 boneIndices;
layout(location = 8) in vec4 boneWeights;

layout(set = 0, binding = 0, std140) uniform SkinningPalette {
    mat4 bones[64];
} skinning;

layout(push_constant) uniform Transform {
    mat4 mvp;
} transform;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

void main() {
    mat4 skin = mat4(0.0);
    skin += skinning.bones[boneIndices.x] * boneWeights.x;
    skin += skinning.bones[boneIndices.y] * boneWeights.y;
    skin += skinning.bones[boneIndices.z] * boneWeights.z;
    skin += skinning.bones[boneIndices.w] * boneWeights.w;

    mat4 instanceTransform = mat4(instanceM0, instanceM1, instanceM2, instanceM3);
    vec4 skinnedPosition = skin * vec4(inPosition, 1.0);
    vec3 skinnedNormal = normalize(mat3(skin) * inNormal);
    vec4 worldPosition = instanceTransform * skinnedPosition;

    gl_Position = transform.mvp * worldPosition;
    outWorldPosition = worldPosition.xyz;
    outNormal = normalize(mat3(instanceTransform) * skinnedNormal);
    outUV = inUV;
}
