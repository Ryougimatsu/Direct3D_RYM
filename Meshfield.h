#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void MeshField_Finalize(void);
void MeshField_Draw(const DirectX::XMMATRIX& mtxW);