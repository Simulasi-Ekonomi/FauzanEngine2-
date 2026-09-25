#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 9) in vec4 inMaterialColor;
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
    mat4 model;
} transform;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec4 outMaterialColor;

void main() {
    mat4 instanceTransform = mat4(instanceM0, instanceM1, instanceM2, instanceM3);
    float weightSum = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
    mat4 skinMatrix = mat4(1.0);
    if (weightSum > 0.000001) {
        skinMatrix =
            skinning.bones[min(boneIndices.x, 63u)] * boneWeights.x +
            skinning.bones[min(boneIndices.y, 63u)] * boneWeights.y +
            skinning.bones[min(boneIndices.z, 63u)] * boneWeights.z +
            skinning.bones[min(boneIndices.w, 63u)] * boneWeights.w;
    }
    vec4 skinnedPosition = skinMatrix * vec4(inPosition, 1.0);
    vec3 skinnedNormal = mat3(skinMatrix) * inNormal;
    mat4 worldMatrix = transform.model * instanceTransform;
    vec4 worldPosition = worldMatrix * skinnedPosition;
    gl_Position = transform.mvp * worldPosition;
    outWorldPosition = worldPosition.xyz;
    outNormal = normalize(mat3(worldMatrix) * skinnedNormal);
    outUV = inUV;
    outMaterialColor = inMaterialColor;
}
