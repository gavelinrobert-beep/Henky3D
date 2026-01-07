// Shadow Pass Pixel Shader (depth-only, empty)

struct PSInput {
    float4 Position : SV_POSITION;
};

void PSMain(PSInput input) {
    // Depth only - no pixel output needed
}
