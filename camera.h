#pragma once
#include <DirectXMath.h>

namespace hal { class DebugText; }

void Camera_Initialize();
void Camera_Initialize(const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& Front,
	const DirectX::XMFLOAT3& Right,
	const DirectX::XMFLOAT3& Up);
void Camera_Finalize();
void Camera_Update(double elapsed_time);

const DirectX::XMFLOAT4X4& Camera_GetMatrix();
const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix();

const DirectX::XMFLOAT3& Camera_GetPosition();
const DirectX::XMFLOAT3& Camera_GetFront();
const DirectX::XMFLOAT3& Camera_GetUp();
const DirectX::XMFLOAT3& Camera_GetRight();

void Debug_Draw();
