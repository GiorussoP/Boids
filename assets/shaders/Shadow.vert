#version 330 core

// Per-vertex attributes
layout(location = 0) in vec3 inPosition;

// Per-instance attributes (mat4 takes 4 locations)
layout(location = 4) in mat4 inInstanceModel;      // Locations 4, 5, 6, 7

uniform mat4 uLightViewProjection;

void main()
{
    // Transform position by instance model then light view-projection
    gl_Position = uLightViewProjection * inInstanceModel * vec4(inPosition, 1.0);
}