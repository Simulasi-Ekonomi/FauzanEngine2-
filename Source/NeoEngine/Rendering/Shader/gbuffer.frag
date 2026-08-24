#version 300 es
// GBuffer Fragment Shader - OpenGL ES 3.0
// Outputs: Position, Normal, Albedo+Metallic, TexCoord+Roughness+AO

precision highp float;

in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoord;
in vec4  FragPosLightSpace;
in mat3  TBN;

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedoMetallic;
layout(location = 3) out vec4 gAoRoughness;

uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uMetallicMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uAOMap;

uniform vec3  uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAO;

uniform bool uUseAlbedoMap;
uniform bool uUseNormalMap;
uniform bool uUseMetallicMap;
uniform bool uUseRoughnessMap;
uniform bool uUseAOMap;

void main()
{
    // GBuffer slot 0: World-space position (RGB) + padding (A)
    gPosition = vec4(FragPos, 1.0);

    // GBuffer slot 1: World-space normal (RGB) + padding (A)
    gNormal = vec4(normalize(Normal), 1.0);

    // Sample textures or use uniform values
    vec3  albedo    = uUseAlbedoMap    ? pow(texture(uAlbedoMap, TexCoord).rgb, vec3(2.2)) : uAlbedo;
    float metallic  = uUseMetallicMap  ? texture(uMetallicMap, TexCoord).r : uMetallic;
    float roughness = uUseRoughnessMap ? texture(uRoughnessMap, TexCoord).r : uRoughness;
    float ao        = uUseAOMap       ? texture(uAOMap, TexCoord).r : uAO;

    // GBuffer slot 2: Albedo (RGB) + Metallic (A)
    gAlbedoMetallic = vec4(albedo, metallic);

    // GBuffer slot 3: AO (R) + Roughness (G) + TexCoord (BA)
    gAoRoughness = vec4(ao, roughness, TexCoord);
}
