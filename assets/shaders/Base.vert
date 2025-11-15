
#version 330 core

// Per-vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// Per-instance attributes (mat4 takes 4 locations)
layout(location = 4) in mat4 inInstanceModel;      // Locations 4, 5, 6, 7
layout(location = 8) in mat4 inInstanceNormal;     // Locations 8, 9, 10, 11
layout(location = 12) in vec3 inInstanceColor;     // Location 12

uniform mat4 uViewProjection;
uniform mat4 uView;
uniform vec3 uCameraPos;

out vec3 fragNormal;
out vec3 fragColor;
out vec3 fragPosition;
out vec2 fragTexCoord;

void main()
{
    // Transform position by instance model then view-projection
    gl_Position = uViewProjection * inInstanceModel * vec4(inPosition, 1.0);
    

    fragPosition = vec3(inInstanceModel * vec4(inPosition, 1.0));
    // Transform normal to world space using instance normal matrix
    fragNormal = mat3(inInstanceNormal) * inNormal;

    fragTexCoord = inTexCoord;
    
    // Pass instance color
    fragColor = inInstanceColor;
}
