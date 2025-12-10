#include "LightCamera.h"
using namespace DirectX;


namespace {
	XMFLOAT3 g_Position{};
	XMFLOAT3 g_Front{ 0.0f, 1.0f, 0.0f };
}
void LightCamera_Initialize(const DirectX::XMFLOAT3& world_directional, const DirectX::XMFLOAT3& position)
{
	g_Front = world_directional;
	g_Position = position;
}

void LightCamera_Finalize()
{
}

void LightCamera_SetPosition(const DirectX::XMFLOAT3& position)
{
	g_Position = position;
}

void LightCamera_SetFront(const DirectX::XMFLOAT3& front)
{
	g_Front = front;
}

const DirectX::XMFLOAT4X4& LightCamera_GetViewMatrix()
{
	XMFLOAT4X4 mtxView;

	// 观察位置: g_Position
	// 观察方向: 垂直向下 (0, -1, 0)
	// 上方向量: g_Front (这通常用于阴影贴图摄像机，以保持方向一致性)
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&g_Position), XMVECTOR{ 0.0f, -1.0f, 0.0f }, XMLoadFloat3(&g_Front));

	XMStoreFloat4x4(&mtxView, view);

	return mtxView;
}

const DirectX::XMFLOAT4X4& LightCamera_GetProjectionMatrix()
{
	XMFLOAT4X4 mtxProj;

	// 参数：ViewLeft, ViewRight, ViewBottom, ViewTop, NearZ, FarZ
	XMMATRIX proj = XMMatrixOrthographicOffCenterLH(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f);

	XMStoreFloat4x4(&mtxProj, proj);

	return mtxProj;
}
