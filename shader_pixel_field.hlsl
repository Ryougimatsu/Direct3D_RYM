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

SamplerState samp : register(s0); // テクスチャサンプラ (Texture Sampler)

float4 main(PS_IN pi) : SV_TARGET
{
    // === 纹理混合 ===
    // 使用顶点颜色的 R 和 G 通道作为 tex1 和 tex0 的混合权重
    float4 tex_color = tex1.Sample(samp, pi.uv) * pi.color.r + tex0.Sample(samp, pi.uv) * pi.color.g;

    // === 光照计算 ===
    // 将环境光和方向光相加，然后与纹理颜色相乘
    // 注意：pi.directional + pi.ambient 的值如果大于1可能会导致过曝，但这是基本光照模型
    float4 final_color = tex_color * (pi.directional + pi.ambient);
    
    // 确保 alpha 通道是 1.0
    final_color.a = 1.0f;

    return final_color;
}