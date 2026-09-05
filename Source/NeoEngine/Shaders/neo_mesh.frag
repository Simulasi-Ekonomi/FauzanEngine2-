#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(inNormal);
    float ndotl = max(dot(n, normalize(vec3(0.35, 0.7, 0.6))), 0.0);
    vec3 base = vec3(0.35 + 0.35 * inUV.x, 0.45 + 0.25 * inUV.y, 0.75);
    vec3 color = base * (0.18 + 0.82 * ndotl);
    outColor = vec4(color, 1.0);
}
