#pragma once
#include <DirectXMath.h>
#include <d3d11.h>

void Billboard_Initialize();
void Billboard_Finalize(void);
void Billboard_Draw(int texID, const DirectX::XMFLOAT3& position,float scale_x,float scale_y,const DirectX::XMFLOAT2& pivot);