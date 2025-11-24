#version 330 core

// Uniforms
uniform vec3 uColor;

// Output color
out vec4 outColor;

void main()
{
    outColor = vec4(uColor, 1.0);
}