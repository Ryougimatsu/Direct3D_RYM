/*==============================================================================

   Direct3D�̏������֘A [direct3d.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/12
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef DIRECT3D_H
#define DIRECT3D_H

#include <d3d11.h>
#include <Windows.h>
#include <DirectXMath.h>



// �Z�[�t�����[�X�}�N��
#define SAFE_RELEASE(o) if (o) { (o)->Release(); o = NULL; }


bool Direct3D_Initialize(HWND hWnd);
void Direct3D_Finalize();

void Direct3D_Clear();
void Direct3D_Present();

unsigned int Direct3D_GetBackBufferWidth();// バックバッファの幅を取得
unsigned int Direct3D_GetBackBufferHeight(); // バックバッファの幅と高さを取得

ID3D11Device* Direct3D_GetDevice();
ID3D11DeviceContext* Direct3D_GetDeviceContext();

void Direct3D_SetDepthEnable(bool enable);
void Direct3D_SetBlendState(bool enable);
void Direct3D_SetDepthStencilStateDepthWriteDisable(bool enalbe);

DirectX::XMMATRIX Direct3D_MatrixViewport();

DirectX::XMFLOAT3 Direct3D_ScreenToWorld(int x, int y, float depth, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);


DirectX::XMFLOAT2 Direct3D_WorldToScreen(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

void Direct3D_ClearBackBuffer();

void Direct3D_SetOffBackBuffer();

void Direct3D_ClearOffScreen();

void Direct3D_SetOffscreen();

void Direct3D_SetOffscreenTexture(int slot);

ID3D11ShaderResourceView* Direct3D_GetOffscreenSRV();
#endif // DIRECT3D_H
