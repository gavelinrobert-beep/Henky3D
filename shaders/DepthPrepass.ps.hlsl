// Depth Prepass Pixel Shader (empty for depth-only pass)

struct PSInput {
    float4 Position : SV_POSITION;
};

void PSMain(PSInput input) {
    // Depth-only pass, no output
}
