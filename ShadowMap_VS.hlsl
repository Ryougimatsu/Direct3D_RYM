cbuffer ConstantBuffer : register(b0)
{
	matrix WorldLightViewProj; // 世界 * 光源View * 光源Proj
}

struct VS_INPUT
{
	float4 Pos : POSITION;
    // 阴影生成不需要法线、颜色或UV
};

float4 main(VS_INPUT input) : SV_POSITION
{
	return mul(input.Pos, WorldLightViewProj);
}