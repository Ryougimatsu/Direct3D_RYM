
// 定数バッファ
cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 world;

}

cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 view;
}

cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4x4 proj;
}

cbuffer VS_CONSTANT_BUFFER : register(b3)
{
    float4 ambient_color;
}

cbuffer VS_CONSTANT_BUFFER : register(b4)
{
    float4 directional_vector;
    float4 directional_color;
}
struct VS_IN
{
    float4 posL : POSITION0; // ローカル座標
    float3 normalL : NORMAL0; // ローカル法線
    float4 color : COLOR0; // 色
    float2 uv : TEXCOORD0; // uv
};

struct VS_OUT
{
    float4 posH : SV_POSITION; // 変換後の座標
    float4 color : COLOR0; // 色
    float4 light_color : COLOR1; // ライトカラー
    float2 uv : TEXCOORD0; // uv
};

//=============================================================================
// 頂点シェ一ダ
//=============================================================================
VS_OUT main(VS_IN vi)
{
    VS_OUT vo;

    float4 pos = float4(vi.posL.xyz, 1.0f); //补w
    pos = mul(pos, world); // 依次 world -> view -> proj
    pos = mul(pos, view);
    vo.posH = mul(pos, proj);

    float4 normalW = mul(float4(vi.normalL.xyz, 0.0f), world);
    normalW = normalize(normalW);
    float dl = max(0,dot(-directional_vector, normalW));
    
    vo.color = vi.color;

    float3 color = vi.color.rgb * dl * directional_color.rgb + ambient_color.rgb * vi.color.rgb;
    vo.light_color = float4(color, 1.0f);
    vo.uv = vi.uv;

    return vo;
}