
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

cbuffer VS_SHADOW_BUFFER : register(b3)
{
	float4x4 lightViewProj;
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
    float4 posW : POSITION0; // ワールド座標
    float4 normalW : NORMAL0; // ワールド法線
    float4 color : COLOR0; // 色
    float2 uv : TEXCOORD0; // uv
	float4 posLight : POSITION1; // 光源ビュー・プロジェクション変換後の座標
};

//=============================================================================
// 頂点シェ一ダ
//=============================================================================
VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    //float4 pos = float4(vi.posL.xyz, 1.0f); //补w
    //pos = mul(pos, world); // 依次 world -> view -> proj
    //pos = mul(pos, view);
    float4x4 mtxWV = mul(world, view);
    float4x4 mtxWVP = mul(mtxWV, proj);
    vo.posH = mul(vi.posL, mtxWVP);

    float4 normalW = mul(float4(vi.normalL.xyz, 0.0f), world);
    vo.normalW = normalize(normalW);
    vo.posW = mul(vi.posL,world);

	vo.posLight = mul(vo.posW, lightViewProj);
    
    vo.color = vi.color;
    vo.uv = vi.uv;

    return vo;
}