struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float4 directional : COLOR1;
    float4 ambient : COLOR2;
    float2 uv : TEXCOORD0;
};

Texture2D tex0 : register(t0); // テクスチャ (Texture)
Texture2D tex1 : register(t1); // テクスチャ (Texture)

SamplerState samp; // テクスチャサンプラ (Texture Sampler)

float4 main(PS_IN pi) : SV_TARGET
{
    float2 uv;
    float angle = 3.14159f * 45 / 180.0f;
    uv.x = pi.uv.x * cos(angle) + pi.uv.y * sin(angle);
    uv.y = -pi.uv.x * sin(angle) + pi.uv.y * cos(angle);

    float4 tex_color = tex0.Sample(samp, pi.uv) * pi.color.g + tex1.Sample(samp, pi.uv) * pi.color.r;
    return tex_color * pi.directional + tex_color * pi.ambient;

    // return tex0.Sample(samp, uv) * 0.5f + tex1.Sample(samp, uv) * 0.5f;
}