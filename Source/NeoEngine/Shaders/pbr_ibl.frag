#version 450

layout(set = 0, binding = 0) uniform samplerCube irradianceMap;
layout(set = 0, binding = 1) uniform samplerCube prefilteredEnvironment;
layout(set = 0, binding = 2) uniform sampler2D brdfLut;

layout(set = 1, binding = 0, std140) uniform IBLParams {
    vec4 cameraPosition;
    vec4 environmentSettings; // intensity, max reflection LOD, irradiance strength, unused
} ibl;

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

void main() {
    vec3 n = normalize(inNormal);
    vec3 v = normalize(ibl.cameraPosition.xyz - inWorldPosition);
    vec3 reflection = reflect(-v, n);

    // The material color/metallic/roughness are supplied by the PBR lighting stage.
    // This pass exposes the split-sum IBL terms: diffuse irradiance and roughness-mipped specular.
    vec3 irradiance = texture(irradianceMap, n).rgb * ibl.environmentSettings.z;
    float roughness = clamp(texture(brdfLut, inUV).g, 0.0, 1.0);
    float lod = roughness * max(ibl.environmentSettings.y, 0.0);
    vec3 prefiltered = textureLod(prefilteredEnvironment, reflection, lod).rgb;

    // Preserve HDR range; tone mapping belongs to the later post-processing stage.
    vec3 iblColor = (irradiance / PI) + prefiltered;
    outColor = vec4(max(iblColor * max(ibl.environmentSettings.x, 0.0), vec3(0.0)), 1.0);
}
