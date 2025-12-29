#pragma once
#include <DirectXMath.h>



void Player_Camera_Initialize();
void Player_Camera_Finalize();
void Player_Camera_Update(double elapsed_time, const DirectX::XMFLOAT3& playerPos);

const DirectX::XMFLOAT3& Player_Camera_GetFront();
const DirectX::XMFLOAT3& Player_Camera_GetPosition();

const DirectX::XMFLOAT4X4& Player_Camera_GetMatrix();
const DirectX::XMFLOAT4X4& Player_Camera_GetViewMatrix();
const DirectX::XMFLOAT4X4& Player_Camera_GetProjectionMatrix();

DirectX::XMVECTOR GetMouseWorldPos(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);

