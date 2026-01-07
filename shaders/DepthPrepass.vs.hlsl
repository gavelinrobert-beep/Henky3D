// Depth Prepass Vertex Shader

struct PerFrameConstants {
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 ViewProjectionMatrix;
    float4 CameraPosition;
    float Time;
    float DeltaTime;
    float2 Padding;
};

struct PerDrawConstants {
    float4x4 WorldMatrix;
    uint MaterialIndex;
    uint3 Padding;
};

ConstantBuffer<PerFrameConstants> g_PerFrame : register(b0);
ConstantBuffer<PerDrawConstants> g_PerDraw : register(b1);

struct VSInput {
    float3 Position : POSITION;
};

struct PSInput {
    float4 Position : SV_POSITION;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float4 worldPos = mul(float4(input.Position, 1.0f), g_PerDraw.WorldMatrix);
    output.Position = mul(worldPos, g_PerFrame.ViewProjectionMatrix);
    
    return output;
}
