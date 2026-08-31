// Shared vertex stage for GYO's unlit mesh and sprite pipelines.
cbuffer VertexUniforms : register(b0, space1)
{
    row_major float4x4 worldViewProjection;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VertexOutput main(float3 position : TEXCOORD0, float2 uv : TEXCOORD1)
{
    VertexOutput output;
    output.position = mul(float4(position, 1.0f), worldViewProjection);
    output.uv = uv;
    return output;
}
