// Shadow Pass Vertex Shader

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
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float4 worldPos = mul(float4(input.Position, 1.0f), g_PerDraw.WorldMatrix);
    output.Position = mul(worldPos, g_PerFrame.LightViewProjectionMatrix);
    
    return output;
}
