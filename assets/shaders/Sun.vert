#version 330 core

// Per-vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

// Uniforms
uniform mat4 uModel;
uniform mat4 uViewProjection;

void main()
{
    // Transform position
    gl_Position = uViewProjection * uModel * vec4(inPosition, 1.0);
}