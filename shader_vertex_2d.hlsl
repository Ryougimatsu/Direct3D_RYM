/*==============================================================================

   2D描画用頂点シエ一ダ一 [shader_vertex_2d.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer VS_CONSTANT_BUFFER : register(b0)
{float4x4 proj;}

cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 world;
}
struct VS_IN
{
    float4 posL : POSITION0; // ローカル座標
    float4 color : COLOR0; // 色
    float2 uv : TEXCOORD0; // UV座標（必要に応じて追加）
};

struct VS_OUT
{
    float4 posH : SV_POSITION; // 変換後の座標
    float4 color : COLOR0; // 色
    float2 uv : TEXCOORD0; // UV座標（必要に応じて追加）
};

//=============================================================================
// 頂点シェ一ダ
//=============================================================================
VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    vi.posL = mul(vi.posL, world);
    vo.posH  = mul(vi.posL, proj); // ローカル座標を変換
    vo.color = vi.color; // 色をそのまま渡す
    vo.uv    = vi.uv; // UV座標をそのまま渡す（必要に応じて追加）
    return vo;
}
