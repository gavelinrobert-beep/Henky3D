// Forward Pass Pixel Shader

#include "Common.hlsli"

ConstantBuffer<PerFrameConstants> g_PerFrame : register(b0);

Texture2D g_ShadowMap : register(t0);
SamplerComparisonState g_ShadowSampler : register(s0);

struct PSInput {
    float4 Position : SV_POSITION;
    float3 WorldPos : POSITION0;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float4 ShadowPos : POSITION1;
};

float SampleShadowMapPCF(float3 shadowPos) {
    // Perspective divide
    shadowPos.xy /= shadowPos.w;
    
    // Transform to [0,1] range
    shadowPos.xy = shadowPos.xy * 0.5f + 0.5f;
    shadowPos.y = 1.0f - shadowPos.y; // Flip Y for texture coordinates
    
    // Check if in shadow map bounds
    if (shadowPos.x < 0.0f || shadowPos.x > 1.0f || 
        shadowPos.y < 0.0f || shadowPos.y > 1.0f ||
        shadowPos.z < 0.0f || shadowPos.z > 1.0f) {
        return 1.0f; // Not in shadow if outside bounds
    }
    
    // Apply bias
    float depth = shadowPos.z - g_PerFrame.ShadowBias;
    
    // Simple PCF (4 samples)
    float2 texelSize = 1.0f / float2(2048.0f, 2048.0f); // TODO: make configurable
    float shadow = 0.0f;
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            shadow += g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, shadowPos.xy + offset, depth);
        }
    }
    
    return shadow / 9.0f;
}

float4 PSMain(PSInput input) : SV_TARGET {
    // Normalize inputs
    float3 N = normalize(input.Normal);
    float3 L = normalize(-g_PerFrame.LightDirection.xyz);
    float3 V = normalize(g_PerFrame.CameraPosition.xyz - input.WorldPos);
    float3 H = normalize(L + V);
    
    // Base color
    float3 baseColor = input.Color.rgb;
    
    // Diffuse lighting
    float NdotL = max(dot(N, L), 0.0f);
    float3 diffuse = baseColor * NdotL * g_PerFrame.LightColor.rgb * g_PerFrame.LightColor.a;
    
    // Specular (simple Blinn-Phong)
    float NdotH = max(dot(N, H), 0.0f);
    float shininess = 32.0f;
    float specular = pow(NdotH, shininess);
    float3 specularColor = float3(1.0f, 1.0f, 1.0f) * specular * 0.3f;
    
    // Ambient
    float3 ambient = baseColor * g_PerFrame.AmbientColor.rgb * g_PerFrame.AmbientColor.a;
    
    // Shadow
    float shadowFactor = 1.0f;
    if (g_PerFrame.ShadowsEnabled > 0.5f) {
        shadowFactor = SampleShadowMapPCF(input.ShadowPos.xyz);
    }
    
    // Combine lighting
    float3 finalColor = ambient + (diffuse + specularColor) * shadowFactor;
    
    return float4(finalColor, input.Color.a);
}
