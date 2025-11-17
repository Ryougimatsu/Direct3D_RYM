#pragma once
#include <DirectXMath.h>
#include <d3d11.h>

void Billboard_Initialize();
void Billboard_Finalize(void);
void Billboard_Draw(int texID, const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT2& scale,
	const DirectX::XMFLOAT2& pivot = { 0.0f,0.0f });
void Billboard_Draw(int texID, 
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT2& scale,
	const DirectX::XMFLOAT2& pivot = {0.0f,0.0f},
	const DirectX::XMFLOAT4& tex_cut
	);
