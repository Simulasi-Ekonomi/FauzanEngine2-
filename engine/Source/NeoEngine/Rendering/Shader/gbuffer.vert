#version 300 es
// GBuffer Vertex Shader - OpenGL ES 3.0
// Deferred rendering geometry pass

precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpaceMatrix;

out vec3  FragPos;
out vec3  Normal;
out vec2  TexCoord;
out vec4  FragPosLightSpace;
out mat3  TBN;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    Normal = normalize(normalMatrix * aNormal);

    TexCoord = aTexCoord;

    FragPosLightSpace = uLightSpaceMatrix * worldPos;

    // Tangent-Bitangent-Normal matrix for normal mapping
    vec3 T = normalize(mat3(uModel) * aTangent);
    vec3 B = normalize(mat3(uModel) * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    TBN = mat3(T, B, N);

    gl_Position = uProjection * uView * worldPos;
}
