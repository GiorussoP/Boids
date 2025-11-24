#version 330 core

// From vertex shader
in vec3 fragNormal;
in vec3 fragColor;
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragShadowCoord;

// Uniforms
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uCameraPos;
uniform bool uIsGround;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform sampler2D uShadowMap;

// Output color
out vec4 outColor;

void main()
{
    // Shadow calculation with PCF
    vec3 projCoords = fragShadowCoord.xyz / fragShadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;
    float currentDepth = projCoords.z;
    float bias = 0.0005;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // Simple Bliin-phong lighting
    vec3 specular = vec3(0.0);
    vec3 viewDir = normalize(uCameraPos - vec3(fragPosition));
    vec3 reflectDir = reflect(uLightDir, fragNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    specular = uLightColor * spec;


    vec3 surfaceColor;
    float mult = smoothstep(1.0, 1.4, fragPosition.y / 300.0);
    vec3 rockColor = (1.0-mult) * vec3(0.6, 0.6, 0.7) + mult * vec3(0.5, 0.7, 1.0);
    if(uIsGround) {
    
        surfaceColor = dot(fragNormal, vec3(0.0, 1.0, 0.0)) > 0.6 ? fragPosition.y < 275.0 ? vec3(0.3, 0.7, 0.3) : vec3(1.0, 1.0, 1.0) : rockColor;
    
    }
    else {
        surfaceColor = fragColor;
    }

    vec3 lighting = uAmbientColor + (1.0 - shadow) * uLightColor * max(dot(fragNormal, -uLightDir), 0.0);
    outColor = vec4(surfaceColor * lighting + specular, 1.0);

    // Fog
    float dist = length(uCameraPos - fragPosition);
    float fogFactor = exp(-uFogDensity * dist);
    outColor.rgb = mix(uFogColor, outColor.rgb, fogFactor);
}
