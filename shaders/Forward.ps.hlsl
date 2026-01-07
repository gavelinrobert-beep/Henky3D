// Forward Pass Pixel Shader

struct PerFrameConstants {
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 ViewProjectionMatrix;
    float4 CameraPosition;
    float Time;
    float DeltaTime;
    float2 Padding;
};

ConstantBuffer<PerFrameConstants> g_PerFrame : register(b0);

struct PSInput {
    float4 Position : SV_POSITION;
    float3 WorldPos : POSITION0;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
};

float4 PSMain(PSInput input) : SV_TARGET {
    // Simple lighting calculation
    float3 N = normalize(input.Normal);
    float3 L = normalize(float3(0.5f, 1.0f, 0.3f)); // Simple directional light
    float3 V = normalize(g_PerFrame.CameraPosition.xyz - input.WorldPos);
    float3 H = normalize(L + V);
    
    // Diffuse
    float NdotL = max(dot(N, L), 0.0f);
    float3 diffuse = input.Color.rgb * NdotL;
    
    // Specular
    float NdotH = max(dot(N, H), 0.0f);
    float specular = pow(NdotH, 32.0f);
    
    // Ambient
    float3 ambient = input.Color.rgb * 0.2f;
    
    float3 finalColor = ambient + diffuse + specular * 0.3f;
    
    return float4(finalColor, input.Color.a);
}
