// Shared fragment stage for GYO's unlit mesh and sprite pipelines.
Texture2D<float4> colorTexture : register(t0, space2);
SamplerState colorSampler : register(s0, space2);

cbuffer FragmentUniforms : register(b0, space3)
{
    float4 tint;
    float4 uvScaleOffset;
    float alphaCutoff;
    float3 fragmentPadding;
};

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    const float2 transformedUv =
        uv * uvScaleOffset.xy + uvScaleOffset.zw;
    const float4 color = colorTexture.Sample(colorSampler, transformedUv) * tint;
    if (alphaCutoff >= 0.0f)
    {
        clip(color.a - alphaCutoff);
    }
    return color;
}
