#version 450

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inMaterialColor;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(inNormal);
    float ndotl = max(dot(n, normalize(vec3(0.35, 0.7, 0.6))), 0.0);
    vec3 base = vec3(0.35 + 0.35 * inUV.x, 0.45 + 0.25 * inUV.y, 0.75);
    float distanceFade = 1.0 / (1.0 + 0.0005 * dot(inWorldPosition, inWorldPosition));
    vec3 color = base * inMaterialColor.rgb * (0.18 + 0.82 * ndotl) * distanceFade;
    outColor = vec4(color, inMaterialColor.a);
}
