#version 450

layout(set = 0, binding = 0) uniform samplerCube irradianceMap;
layout(set = 0, binding = 1) uniform samplerCube prefilteredEnvironment;
layout(set = 0, binding = 2) uniform sampler2D brdfLut;

layout(set = 1, binding = 0, std140) uniform IBLParams {
    vec4 cameraPosition;
    vec4 environmentSettings; // intensity, max reflection LOD, irradiance strength, unused
    vec4 materialParameters; // metallic, roughness, unused, unused
    vec4 albedo;
} ibl;

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

void main() {
    vec3 n = normalize(inNormal);
    vec3 v = normalize(ibl.cameraPosition.xyz - inWorldPosition);
    float nDotV = max(dot(n, v), 0.0);
    vec3 reflection = reflect(-v, n);

    float metallic = clamp(ibl.materialParameters.x, 0.0, 1.0);
    float roughness = clamp(ibl.materialParameters.y, 0.045, 1.0);
    vec3 baseColor = max(ibl.albedo.rgb, vec3(0.0));
    vec3 f0 = mix(vec3(0.04), baseColor, metallic);

    vec3 irradiance = texture(irradianceMap, n).rgb * max(ibl.environmentSettings.z, 0.0);
    vec3 diffuse = irradiance * baseColor / PI * (1.0 - metallic);

    float lod = roughness * max(ibl.environmentSettings.y, 0.0);
    vec3 prefiltered = textureLod(prefilteredEnvironment, reflection, lod).rgb;
    vec2 brdf = texture(brdfLut, vec2(nDotV, roughness)).rg;
    vec3 fresnel = f0 + (1.0 - f0) * pow(1.0 - nDotV, 5.0);
    vec3 specular = prefiltered * (fresnel * brdf.x + brdf.y);

    vec3 iblColor = (diffuse + specular) * max(ibl.environmentSettings.x, 0.0);
    outColor = vec4(max(iblColor, vec3(0.0)), 1.0);
}
