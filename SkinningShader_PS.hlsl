//-----------------------------------------------------------------------------
// Constant Buffers
//-----------------------------------------------------------------------------

// Slot: b0 - 材质颜色
cbuffer CB_MATERIAL : register(b0)
{
    float4 g_MaterialColor;
}

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------

Texture2D g_Texture : register(t0); // 漫反射贴图
SamplerState g_Sampler : register(s0); // 纹理采样器

// [新增] 阴影资源
Texture2D g_ShadowMap : register(t1); // 阴影深度图 (对应 C++ SetShaderResources(1, ...))
SamplerComparisonState g_ShadowSampler : register(s1); // [重要] 阴影比较采样器 (需要 C++ 创建并绑定到 slot 1)

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
    float4 ShadowPos : TEXCOORD1; // [新增] 必须与 VS 输出一致
};

//-----------------------------------------------------------------------------
// Helper Function: 计算阴影因子
// 返回 1.0 (无阴影) ~ 0.0 (全阴影)
//-----------------------------------------------------------------------------
float CalcShadowFactor(float4 shadowPos)
{
    // 1. 透视除法 (将坐标归一化到 [-1, 1])
    float3 projCoords = shadowPos.xyz / shadowPos.w;

    // 2. 将 [-1, 1] 映射到 UV 空间 [0, 1]
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f; // DX11 Y轴向下，需要翻转

    // 3. 边界检查：如果超出阴影图范围，视为未被遮挡
    if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f ||
        projCoords.z > 1.0f)
    {
        return 1.0f;
    }

    // 4. 深度偏移 (Bias) 防止阴影波纹 (Shadow Acne)
    float bias = 0.001f;
    float currentDepth = projCoords.z - bias;

    // 5. PCF 采样 (使用 SampleCmpLevelZero 进行硬件比较过滤)
    // 比较逻辑：如果 ShadowMap.depth >= currentDepth，则返回 1，否则返回 0
    // 结果会被线性插值，产生柔和边缘
    return g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, projCoords.xy, currentDepth);
}

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
float4 main(PS_IN pin) : SV_TARGET
{
    // 1. 采样基础纹理
    float4 texColor = g_Texture.Sample(g_Sampler, pin.uv);
    
    // 原始逻辑保留：处理透明/丢失材质
    if (texColor.a < 0.01f)
    {
        texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        clip(texColor.a - 0.1f);
    }

    // 2. 准备光照向量
    float3 normal = normalize(pin.normalW.xyz);
    // [注意] 这里的硬编码光照方向应该与生成阴影的光源方向一致，否则阴影会“错位”
    float3 lightDir = normalize(float3(0.5f, -1.0f, 0.5f));

    // 3. 计算基础漫反射 (Half Lambert)
    float NdotL = saturate(dot(normal, -lightDir));
    float halfLambert = NdotL * 0.5f + 0.5f;

    // 4. [新增] 计算阴影遮挡
    float shadowFactor = CalcShadowFactor(pin.ShadowPos);

    // 5. 合成最终颜色
    // 公式策略：Ambient + (Diffuse * Shadow)
    // 你的代码使用了 0.2f 作为环境光底色，halfLambert 作为漫反射
    // 我们只让阴影影响 halfLambert 部分，保留 0.2f 的环境亮度，避免阴影处死黑
    float3 lighting = (0.2f + halfLambert * shadowFactor);

    float4 finalColor;
    finalColor.rgb = texColor.rgb * g_MaterialColor.rgb * lighting;
    finalColor.a = texColor.a;

    return finalColor;
}