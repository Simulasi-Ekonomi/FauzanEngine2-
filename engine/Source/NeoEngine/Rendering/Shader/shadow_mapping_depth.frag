#version 300 es
// Shadow Mapping Depth Fragment Shader - OpenGL ES 3.0
// Minimal — just writes depth (no color output needed, but GLES requires it)

precision highp float;

void main()
{
    // Depth is automatically written by the rasterizer
    // GLES 3.0 fragment shader must output something
    gl_FragDepth = gl_FragCoord.z;
}
