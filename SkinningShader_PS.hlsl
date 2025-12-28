//-----------------------------------------------------------------------------
// Constant Buffers
//-----------------------------------------------------------------------------

// Slot: b0 - 材质颜色
cbuffer CB_MATERIAL : register(b0)
{
    float4 g_MaterialColor; // 由 C++ 传入，通常包含模型自带的 Diffuse Color
}

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------

Texture2D g_Texture : register(t0); // 漫反射贴图
SamplerState g_Sampler : register(s0); // 采样器状态

//-----------------------------------------------------------------------------
// Input Structure
//-----------------------------------------------------------------------------
struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
float4 main(PS_IN pin) : SV_TARGET
{
    float4 texColor = g_Texture.Sample(g_Sampler, pin.uv);
    
    // 逻辑：如果采样结果是全黑/全透明，且存在材质颜色，则使用白色作为贴图基色
    if (texColor.a < 0.01f)
    {
        texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        // 只有在确实有贴图时才执行透明剔除
        clip(texColor.a - 0.1f);
    }

    float3 normal = normalize(pin.normalW.xyz);
    float3 lightDir = normalize(float3(0.5f, -1.0f, 0.5f));
    float NdotL = saturate(dot(normal, -lightDir));
    float halfLambert = NdotL * 0.5f + 0.5f; // 半兰伯特光照，增加暗部细节

    float4 finalColor;
    // 最终颜色 = 贴图 * 材质基色 * 光照
    finalColor.rgb = texColor.rgb * g_MaterialColor.rgb * (0.2f + halfLambert);
    finalColor.a = texColor.a;

    return finalColor;
}