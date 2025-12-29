/*==============================================================================

   Billboard描画用ピクセルシェーダー [shader_pixel_Billboard.hlsl]

==============================================================================*/
struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0; // 顶点颜色 (保持不变，兼容旧功能)
    float2 uv : TEXCOORD0;
};

// 对应 C++ 里 g_pPSConstantBuffer0（Shader_Billboard_SetColor）
cbuffer PSColorBuffer : register(b0)
{
    float4 g_Color; // (r,g,b,a) 统一的颜色 tint
};

Texture2D tex : register(t0);
SamplerState samplerState : register(s0);

float4 main(PS_IN ps_in) : SV_TARGET
{
    // 采样纹理
    float4 texColor = tex.Sample(samplerState, ps_in.uv);

    // 最终颜色 = 纹理 * 顶点颜色 * 全局颜色
    // 其它 2D：顶点白色 × g_Color = (1,1,1,1) 时，行为与原来完全一样
    float4 outColor = texColor * ps_in.color * g_Color;

    return outColor;
}
