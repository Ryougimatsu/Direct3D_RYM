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

enum BLEND_MODE
{
	BLEND_MODE_NONE,   // 不混合 (Opaque)
	BLEND_MODE_ALPHA,  // 普通透明混合 (SrcAlpha, InvSrcAlpha) - 用于烟雾、UI
	BLEND_MODE_ADD,    // 加法混合 (SrcAlpha, One) - 用于火焰、光效、粒子
};

bool Direct3D_Initialize(HWND hWnd);
void Direct3D_Finalize();

void Direct3D_Clear();
void Direct3D_Present();

unsigned int Direct3D_GetBackBufferWidth();// バックバッファの幅を取得
unsigned int Direct3D_GetBackBufferHeight(); // バックバッファの幅と高さを取得
HWND Direct3D_GetWindowHandle();

ID3D11Device* Direct3D_GetDevice();
ID3D11DeviceContext* Direct3D_GetDeviceContext();

void Direct3D_SetDepthEnable(bool enable);
void Direct3D_SetBlendState(bool enable);
void Direct3D_SetBlendState(BLEND_MODE mode);
void Direct3D_SetDepthStencilStateDepthWriteDisable(bool enalbe);

void Direct3D_ClearBackBuffer();

void Direct3D_SetOffBackBuffer();
#endif // DIRECT3D_H
