#version 330 core

// From vertex shader
flat in vec3 fragNormal;
in vec3 fragColor;
in vec3 fragPosition;

// Uniforms
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uCameraPos;

out vec4 outColor;

void main()
{

    // Simple Bliin-phong lighting
    vec3 specular = vec3(0.0);
    vec3 viewDir = normalize(uCameraPos - vec3(fragPosition));
    vec3 reflectDir = reflect(uLightDir, fragNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    specular = uLightColor * spec;

    outColor = vec4(fragColor * (uAmbientColor + uLightColor * max(dot(fragNormal, -uLightDir), 0.0)) + specular, 1.0);

    //outColor = vec4(specular,1.0);

    //outColor = vec4(uAmbientColor + uLightColor * max(dot(fragNormal, -uLightDir), 0.0), 1.0);
}
