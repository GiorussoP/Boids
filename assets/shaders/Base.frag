#version 330 core

// From vertex shader
in vec3 fragNormal;
in vec3 fragColor;
in vec3 fragPosition;
in vec2 fragTexCoord;

// Uniforms
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uCameraPos;
uniform bool uIsGround;

// Output color
out vec4 outColor;

void main()
{

    // Simple Bliin-phong lighting
    vec3 specular = vec3(0.0);
    vec3 viewDir = normalize(uCameraPos - vec3(fragPosition));
    vec3 reflectDir = reflect(uLightDir, fragNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    specular = uLightColor * spec;


    vec3 finalColor;
    if(uIsGround) {
    
        finalColor = dot(fragNormal, vec3(0.0, 1.0, 0.0)) > 0.6 ? fragPosition.y < 250.0 ? vec3(0.3, 0.7, 0.3) : vec3(1.0, 1.0, 1.0) : vec3(0.6, 0.6, 0.7);
    
    }
    else {
        finalColor = fragColor;
    }

    outColor = vec4(finalColor * (uAmbientColor + uLightColor * max(dot(fragNormal, -uLightDir), 0.0)) + specular, 1.0);
}
