// Forward Pass Vertex Shader

#include "Common.hlsli"

ConstantBuffer<PerFrameConstants> g_PerFrame : register(b0);
ConstantBuffer<PerDrawConstants> g_PerDraw : register(b1);

struct VSInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float3 WorldPos : POSITION0;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float4 ShadowPos : POSITION1;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float4 worldPos = mul(float4(input.Position, 1.0f), g_PerDraw.WorldMatrix);
    output.WorldPos = worldPos.xyz;
    output.Position = mul(worldPos, g_PerFrame.ViewProjectionMatrix);
    
    // Transform normal to world space (assuming uniform scale)
    output.Normal = mul(input.Normal, (float3x3)g_PerDraw.WorldMatrix);
    output.Color = input.Color;
    
    // Compute shadow map coordinates
    output.ShadowPos = mul(worldPos, g_PerFrame.LightViewProjectionMatrix);
    
    return output;
}
