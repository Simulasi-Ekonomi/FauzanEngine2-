#version 300 es
// Lighting Fragment Shader - OpenGL ES 3.0
// Deferred rendering lighting pass with PBR (Cook-Torrance)

precision highp float;

in vec2 TexCoord;

out vec4 FragColor;

// GBuffer textures
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoMetallic;
uniform sampler2D gAoRoughness;
uniform sampler2D uShadowMap;

// Lighting
struct Light {
    vec3  position;
    vec3  color;
    float intensity;
    vec3  direction;
    float radius;
    int   type;
};

uniform Light uLights[4];
uniform int   uLightCount;
uniform vec3  uCameraPos;
uniform bool  uUseShadowMap;

const float PI = 3.14159265359;

// ---- PBR Distribution functions ----

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return num / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ---- Shadow mapping (deferred version) ----

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir)
{
    // Reconstruct clip-space position for shadow map lookup
    // In deferred, we use world position and light space matrix
    // This is a simplified version; production code would pass light-space position via GBuffer
    return 0.0;
}

// ---- Per-light radiance (same as forward PBR) ----

vec3 CalcLightRadiance(Light light, vec3 fragPos, vec3 N, vec3 V, vec3 F0,
                       vec3 albedo, float metallic, float roughness)
{
    vec3 L;
    vec3 radiance;

    if (light.type == 0) // Directional
    {
        L = normalize(-light.direction);
        radiance = light.color * light.intensity;
    }
    else if (light.type == 1) // Point
    {
        vec3 toLight = light.position - fragPos;
        float dist = length(toLight);
        L = normalize(toLight);

        float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        radiance = light.color * light.intensity * attenuation;
    }
    else // Spot
    {
        vec3 toLight = light.position - fragPos;
        float dist = length(toLight);
        L = normalize(toLight);

        float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        float theta = dot(L, normalize(-light.direction));
        float epsilon = cos(radians(12.5)) - cos(radians(17.5));
        float spotIntensity = clamp((theta - cos(radians(17.5))) / epsilon, 0.0, 1.0);
        radiance = light.color * light.intensity * attenuation * spotIntensity;
    }

    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{
    // Read GBuffer
    vec3  fragPos   = texture(gPosition, TexCoord).rgb;
    vec3  N         = texture(gNormal, TexCoord).rgb;
    vec4  albedoMet = texture(gAlbedoMetallic, TexCoord);
    vec4  aoRough   = texture(gAoRoughness, TexCoord);

    vec3  albedo    = albedoMet.rgb;
    float metallic  = albedoMet.a;
    float ao        = aoRough.r;
    float roughness = aoRough.g;

    N = normalize(N);
    vec3 V = normalize(uCameraPos - fragPos);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Direct lighting
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < uLightCount && i < 4; ++i)
    {
        Lo += CalcLightRadiance(uLights[i], fragPos, N, V, F0, albedo, metallic, roughness);
    }

    // Ambient (IBL approximation)
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = vec3(0.03) * albedo;
    vec3 ambient    = (kD * irradiance) * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping (ACES approximation)
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
