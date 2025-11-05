#pragma once
#ifndef CUBE_H
#define CUBE_H

#include <d3d11.h>
#include <DirectXMath.h>
#include "collision.h"

void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Cube_Finalize(void);
void Cube_Draw(const DirectX ::XMMATRIX mtxW);

AABB Cube_CreateAABB(const DirectX::XMFLOAT3& position);
#endif