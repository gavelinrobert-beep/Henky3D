// Forward Pass Fragment Shader
#version 460 core

#include "Common.glsl"

uniform sampler2DShadow uShadowMap;

in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vColor;
in vec4 vShadowPos;

out vec4 FragColor;

float SampleShadowMapPCF(vec3 shadowPos) {
    // Perspective divide
    shadowPos.xy /= shadowPos.w;
    
    // Transform to [0,1] range
    shadowPos.xy = shadowPos.xy * 0.5 + 0.5;
    
    // Check if in shadow map bounds
    if (shadowPos.x < 0.0 || shadowPos.x > 1.0 || 
        shadowPos.y < 0.0 || shadowPos.y > 1.0 ||
        shadowPos.z < 0.0 || shadowPos.z > 1.0) {
        return 1.0; // Not in shadow if outside bounds
    }
    
    // Apply bias
    float depth = shadowPos.z - ShadowBias;
    
    // Simple PCF (9 samples)
    vec2 texelSize = 1.0 / vec2(2048.0, 2048.0); // TODO: make configurable
    float shadow = 0.0;
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(x, y) * texelSize;
            shadow += texture(uShadowMap, vec3(shadowPos.xy + offset, depth));
        }
    }
    
    return shadow / 9.0;
}

void main() {
    // Normalize inputs
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-LightDirection.xyz);
    vec3 V = normalize(CameraPosition.xyz - vWorldPos);
    vec3 H = normalize(L + V);
    
    // Base color
    vec3 baseColor = vColor.rgb;
    
    // Diffuse lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = baseColor * NdotL * LightColor.rgb * LightColor.a;
    
    // Specular (simple Blinn-Phong)
    float NdotH = max(dot(N, H), 0.0);
    float shininess = 32.0;
    float specular = pow(NdotH, shininess);
    vec3 specularColor = vec3(1.0, 1.0, 1.0) * specular * 0.3;
    
    // Ambient
    vec3 ambient = baseColor * AmbientColor.rgb * AmbientColor.a;
    
    // Shadow
    float shadowFactor = 1.0;
    if (ShadowsEnabled > 0.5) {
        shadowFactor = SampleShadowMapPCF(vShadowPos.xyz);
    }
    
    // Combine lighting
    vec3 finalColor = ambient + (diffuse + specularColor) * shadowFactor;
    
    FragColor = vec4(finalColor, vColor.a);
}
