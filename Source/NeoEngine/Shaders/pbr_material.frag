#version 450

layout(set = 0, binding = 0, std140) uniform MaterialParams {
    vec4 baseColor;
    vec4 materialFactors; // metallic, roughness, normal strength, AO strength
    uint flags;
} material;

layout(set = 0, binding = 1) uniform sampler2D baseColorMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D metallicMap;
layout(set = 0, binding = 4) uniform sampler2D roughnessMap;
layout(set = 0, binding = 5) uniform sampler2D aoMap;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 baseSample = texture(baseColorMap, inUV);
    float metallic = clamp(material.materialFactors.x * texture(metallicMap, inUV).r, 0.0, 1.0);
    float roughness = clamp(material.materialFactors.y * texture(roughnessMap, inUV).r, 0.045, 1.0);
    float ao = mix(1.0, texture(aoMap, inUV).r, clamp(material.materialFactors.w, 0.0, 1.0));

    // PBR-1 material stage: preserve material response for the lighting stage.
    // Lighting, IBL and normal mapping are integrated by the subsequent PBR stages.
    vec3 albedo = material.baseColor.rgb * baseSample.rgb;
    vec3 n = normalize(inNormal);
    float materialResponse = 0.5 + 0.5 * n.z;
    float energy = mix(1.0, 0.96, metallic) * mix(1.0, 0.9, roughness);
    outColor = vec4(albedo * ao * materialResponse * energy, baseSample.a * material.baseColor.a);
}
