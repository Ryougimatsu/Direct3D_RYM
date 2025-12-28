//=============================================================================
// 3D Skinning Vertex Shader (with bones)
//=============================================================================

cbuffer CBWorld : register(b0)
{
    float4x4 gWorld;
};

cbuffer CBView : register(b1)
{
    float4x4 gView;
};

cbuffer CBProj : register(b2)
{
    float4x4 gProj;
};

// 对应 C++ 的 g_pCBBones / MAX_BONES = 256
cbuffer CBBones : register(b3)
{
    float4x4 gBones[256];
};

// VS 输入：和 C++ InputLayout 一致
struct VS_IN
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD0;
    uint4 Indices : BLENDINDICES0;
    float4 Weights : BLENDWEIGHT0;
};

// VS 输出：和 Pixel Shader 的 PS_IN 一致
struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

PS_IN main(VS_IN vin)
{
    PS_IN vout;

    float4 localPos = float4(vin.Pos, 1.0f);
    float3 localN = vin.Normal;

    // ----------- 骨骼蒙皮 -----------
    float4 skinnedPos = float4(0, 0, 0, 0);
    float3 skinnedN = float3(0, 0, 0);

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        uint idx = vin.Indices[i];
        float w = vin.Weights[i];

        if (w > 0.0001f)
        {
            // C++ 那边对 boneMatrices 做了 XMMatrixTranspose，
            // 这里用 mul(pos, gBones[idx]) 正好对应
            skinnedPos += mul(localPos, gBones[idx]) * w;

            float3x3 rot = (float3x3) gBones[idx];
            skinnedN += mul(localN, rot) * w;
        }
    }

    // 如果没有任何有效权重（防御），退回原始位置/法线
    if (all(skinnedPos == 0))
    {
        skinnedPos = localPos;
        skinnedN = localN;
    }

    skinnedN = normalize(skinnedN);

    // ----------- 世界 / 视图 / 投影 -----------
    float4 worldPos = mul(skinnedPos, gWorld);
    float3 worldN = normalize(mul(skinnedN, (float3x3) gWorld));

    float4 viewPos = mul(worldPos, gView);
    float4 projPos = mul(viewPos, gProj);

    vout.posH = projPos;
    vout.posW = worldPos;
    vout.normalW = float4(worldN, 0.0f);
    vout.color = vin.Color;
    vout.uv = vin.Tex;

    return vout;
}
