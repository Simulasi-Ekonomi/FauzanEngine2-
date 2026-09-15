#version 450

layout(set = 0, binding = 0, std140) uniform MaterialParams {
    vec4 baseColor;
    vec4 materialFactors;
    uint flags;
} material;
layout(set = 0, binding = 1) uniform sampler2D baseColorMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D metallicMap;
layout(set = 0, binding = 4) uniform sampler2D roughnessMap;
layout(set = 0, binding = 5) uniform sampler2D aoMap;

struct DirectionalLight { vec4 directionIntensity; vec4 color; };
struct PointLight { vec4 positionRadius; vec4 colorIntensity; };
struct SpotLight { vec4 positionRadius; vec4 directionInnerCos; vec4 colorIntensity; vec4 outerCosPadding; };

layout(set = 1, binding = 0, std140) uniform LightingFrame {
    vec4 cameraPosition;
    vec4 ambientColor;
    uint directionalCount;
    uint pointCount;
    uint spotCount;
    uint _padding;
    DirectionalLight directionalLights[4];
    PointLight pointLights[256];
    SpotLight spotLights[64];
} lighting;

layout(set = 2, binding = 0) uniform samplerCube irradianceMap;
layout(set = 2, binding = 1) uniform samplerCube prefilteredEnvironment;
layout(set = 2, binding = 2) uniform sampler2D brdfLut;

// Push-constant layout shares the first 64 bytes with the existing vertex
// transform block. The fragment stage consumes only the trailing vec4.
layout(push_constant) uniform PBRPushConstants {
    mat4 transformPadding;
    vec4 iblSettings;
} pushConstants;

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;
const float MIN_ROUGHNESS = 0.045;
const float EPSILON = 1e-5;

float DistributionGGX(float nDotH, float roughness) {
    float a = max(roughness, MIN_ROUGHNESS);
    float a2 = a * a;
    float n2 = clamp(nDotH, 0.0, 1.0);
    float denominator = n2 * n2 * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denominator * denominator, EPSILON);
}
float GeometrySchlickGGX(float nDotV, float roughness) {
    float r = max(roughness, MIN_ROUGHNESS);
    float k = ((r + 1.0) * (r + 1.0)) / 8.0;
    float nV = clamp(nDotV, 0.0, 1.0);
    return nV / max(nV * (1.0 - k) + k, EPSILON);
}
float GeometrySmith(float nDotV, float nDotL, float roughness) {
    return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}
vec3 FresnelSchlick(float cosTheta, vec3 f0) {
    float c = clamp(cosTheta, 0.0, 1.0);
    return f0 + (1.0 - f0) * pow(1.0 - c, 5.0);
}

vec3 BuildNormalFromMap(vec3 geometricNormal) {
    vec3 n = normalize(geometricNormal);
    vec3 dp1 = dFdx(inWorldPosition);
    vec3 dp2 = dFdy(inWorldPosition);
    vec2 duv1 = dFdx(inUV);
    vec2 duv2 = dFdy(inUV);
    vec3 tangent = dp1 * duv2.y - dp2 * duv1.y;
    vec3 bitangent = -dp1 * duv2.x + dp2 * duv1.x;
    float tangentLength = length(tangent);
    float bitangentLength = length(bitangent);
    if (tangentLength <= EPSILON || bitangentLength <= EPSILON) {
        vec3 reference = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
        tangent = normalize(cross(reference, n));
        bitangent = normalize(cross(n, tangent));
    } else {
        tangent = normalize(tangent - n * dot(n, tangent));
        bitangent = normalize(cross(n, tangent));
    }
    vec3 tangentNormal = texture(normalMap, inUV).xyz * 2.0 - 1.0;
    tangentNormal.xy *= max(material.materialFactors.z, 0.0);
    tangentNormal = normalize(tangentNormal);
    return normalize(tangent * tangentNormal.x + bitangent * tangentNormal.y + n * tangentNormal.z);
}

vec3 EvaluateBRDF(vec3 n, vec3 v, vec3 l, vec3 radiance, vec3 albedo, float metallic, float roughness) {
    float nDotL = max(dot(n, l), 0.0);
    float nDotV = max(dot(n, v), 0.0);
    if (nDotL <= 0.0 || nDotV <= 0.0) return vec3(0.0);
    vec3 h = normalize(v + l);
    float nDotH = max(dot(n, h), 0.0);
    float hDotV = max(dot(h, v), 0.0);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(hDotV, f0);
    float G = GeometrySmith(nDotV, nDotL, roughness);
    float D = DistributionGGX(nDotH, roughness);
    vec3 specular = (D * G * F) / max(4.0 * nDotV * nDotL, EPSILON);
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;
    return (diffuse + specular) * radiance * nDotL;
}
vec3 EvaluateDirectional(vec3 n, vec3 v, vec3 albedo, float metallic, float roughness, DirectionalLight light) {
    vec3 l = normalize(-light.directionIntensity.xyz);
    vec3 radiance = light.color.rgb * max(light.directionIntensity.w, 0.0);
    return EvaluateBRDF(n, v, l, radiance, albedo, metallic, roughness);
}
vec3 EvaluatePoint(vec3 worldPosition, vec3 n, vec3 v, vec3 albedo, float metallic, float roughness, PointLight light) {
    vec3 toLight = light.positionRadius.xyz - worldPosition;
    float distanceToLight = length(toLight);
    if (distanceToLight <= EPSILON || distanceToLight >= light.positionRadius.w) return vec3(0.0);
    vec3 l = toLight / distanceToLight;
    float radiusFade = 1.0 - smoothstep(0.0, light.positionRadius.w, distanceToLight);
    float attenuation = radiusFade * radiusFade / max(distanceToLight * distanceToLight, 0.01);
    vec3 radiance = light.colorIntensity.rgb * max(light.colorIntensity.w, 0.0) * attenuation;
    return EvaluateBRDF(n, v, l, radiance, albedo, metallic, roughness);
}
vec3 EvaluateSpot(vec3 worldPosition, vec3 n, vec3 v, vec3 albedo, float metallic, float roughness, SpotLight light) {
    vec3 toLight = light.positionRadius.xyz - worldPosition;
    float distanceToLight = length(toLight);
    if (distanceToLight <= EPSILON || distanceToLight >= light.positionRadius.w) return vec3(0.0);
    vec3 l = toLight / distanceToLight;
    float cone = dot(normalize(light.directionInnerCos.xyz), -l);
    float coneRange = max(light.directionInnerCos.w - light.outerCosPadding.x, EPSILON);
    float coneFade = clamp((cone - light.outerCosPadding.x) / coneRange, 0.0, 1.0);
    float radiusFade = 1.0 - smoothstep(0.0, light.positionRadius.w, distanceToLight);
    float attenuation = radiusFade * radiusFade * coneFade / max(distanceToLight * distanceToLight, 0.01);
    vec3 radiance = light.colorIntensity.rgb * max(light.colorIntensity.w, 0.0) * attenuation;
    return EvaluateBRDF(n, v, l, radiance, albedo, metallic, roughness);
}

vec3 EvaluateIBL(vec3 n, vec3 v, vec3 albedo, float metallic, float roughness, float ao) {
    float environmentIntensity = max(pushConstants.iblSettings.x, 0.0);
    float irradianceStrength = max(pushConstants.iblSettings.y, 0.0);
    float nDotV = max(dot(n, v), 0.0);
    vec3 reflection = reflect(-v, n);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = FresnelSchlick(nDotV, f0);
    vec3 irradiance = texture(irradianceMap, n).rgb * irradianceStrength;
    vec3 diffuse = irradiance * albedo / PI * (1.0 - metallic) * ao;
    float queriedMaxLod = max(float(textureQueryLevels(prefilteredEnvironment) - 1), 0.0);
    float maxLod = min(queriedMaxLod, max(pushConstants.iblSettings.z, 0.0));
    float lod = min(roughness * maxLod, queriedMaxLod);
    vec3 prefiltered = textureLod(prefilteredEnvironment, reflection, lod).rgb;
    vec2 brdf = texture(brdfLut, vec2(nDotV, roughness)).rg;
    vec3 specular = prefiltered * (fresnel * brdf.x + brdf.y);
    return (diffuse + specular) * environmentIntensity;
}

void main() {
    vec4 baseSample = texture(baseColorMap, inUV);
    vec3 albedo = material.baseColor.rgb * baseSample.rgb;
    float metallic = clamp(material.materialFactors.x * texture(metallicMap, inUV).r, 0.0, 1.0);
    float roughness = clamp(material.materialFactors.y * texture(roughnessMap, inUV).r, MIN_ROUGHNESS, 1.0);
    float ao = mix(1.0, texture(aoMap, inUV).r, clamp(material.materialFactors.w, 0.0, 1.0));
    vec3 n = BuildNormalFromMap(inNormal);
    vec3 v = normalize(lighting.cameraPosition.xyz - inWorldPosition);
    if (length(v) <= EPSILON) v = n;
    vec3 color = lighting.ambientColor.rgb * albedo * ao;
    for (uint i = 0u; i < min(lighting.directionalCount, 4u); ++i)
        color += EvaluateDirectional(n, v, albedo, metallic, roughness, lighting.directionalLights[i]);
    for (uint i = 0u; i < min(lighting.pointCount, 256u); ++i)
        color += EvaluatePoint(inWorldPosition, n, v, albedo, metallic, roughness, lighting.pointLights[i]);
    for (uint i = 0u; i < min(lighting.spotCount, 64u); ++i)
        color += EvaluateSpot(inWorldPosition, n, v, albedo, metallic, roughness, lighting.spotLights[i]);
    color += EvaluateIBL(n, v, albedo, metallic, roughness, ao);
    outColor = vec4(max(color, vec3(0.0)), baseSample.a * material.baseColor.a);
}
