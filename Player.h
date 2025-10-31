#pragma once

#include <d3d11.h>
#include <DirectXMath.h>


void Player_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3 front);
void Player_Finalize(void);
void Player_Update(double elapsed_time);
void Player_Draw();

const DirectX::XMFLOAT3& Player_GetPosition();
const DirectX::XMFLOAT3& Player_GetFront();