#pragma once
#include <DirectXMath.h>
#include <d3d11.h>

void Billboard_Initialize();
void Billboard_Finalize(void);
void Billboard_Draw(int texID, const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT2& scale,
	const DirectX::XMFLOAT2& pivot = { 0.0f,0.0f });
void Billboard_Draw(
	int texID, 
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT2& scale,
	const DirectX::XMFLOAT4& tex_cut,
	const DirectX::XMFLOAT2& pivot = {0.0f,0.0f}
	);
void Billboard_Draw(int texID, DirectX::XMFLOAT3 pos, float width, float height, float u, float v, float uw, float vh);
void Laser_Billboard_Draw(int texid, DirectX::XMVECTOR start, DirectX::XMVECTOR end, float width);
void Billboard_Draw(
	int texID,
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT2& scale,
	const DirectX::XMFLOAT2& pivot = { 0.0f,0.0f },
	const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f }
);