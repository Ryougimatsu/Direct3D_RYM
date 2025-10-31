#include "Player_Camera.h"
#include <DirectXMath.h>
#include "key_logger.h"
#include "debug_text.h"
#include <sstream>
#include "shader_3d.h"
#include "direct3d.h"
#include "Player.h"

namespace {
	DirectX::XMFLOAT3 g_CameraFront = { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 g_CameraPosition = { 0.0f, 0.0f, 0.0f };
}


void Player_Camera_Initialize()
{

}

void Player_Camera_Finalize()
{
}

void Player_Camera_Update(double elapsed_time)
{
	DirectX::XMVECTOR position = DirectX::XMVectorSubtract(
		DirectX::XMLoadFloat3(&Player_GetPosition()),
		DirectX::XMVectorScale(DirectX::XMLoadFloat3(&Player_GetFront()), 5.0f)
	);

	DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&Player_GetPosition());

	DirectX::XMVECTOR front = DirectX::XMVector3Normalize(
		DirectX::XMVectorSubtract(target, position)
	);
	DirectX::XMStoreFloat3(&g_CameraPosition, position);
	DirectX::XMStoreFloat3(&g_CameraFront, front);


	DirectX::XMMATRIX mtxView = DirectX::XMMatrixLookAtLH(
		position,
		target,
		{0.0f,1.0f,0.0f});

	//DirectX::XMStoreFloat4x4(&g_CameraMatrix, mtxView);
	Shader_3D_SetViewMatrix(mtxView);

	constexpr float fovAngleY = DirectX::XMConvertToRadians(60.0f);
	float aspectRatio = static_cast<float>(Direct3D_GetBackBufferWidth()) / static_cast<float>(Direct3D_GetBackBufferHeight());
	float nearZ = 0.1f;
	float farZ = 100.0f;

	DirectX::XMMATRIX mtxPerspective = DirectX::XMMatrixPerspectiveFovLH(
		fovAngleY,
		aspectRatio,
		nearZ,
		farZ
	);

	//XMStoreFloat4x4(&g_PerspectiveMatrix, mtxPerspective);
	Shader_3D_SetProjectMatrix(mtxPerspective);
}

const DirectX::XMFLOAT3& Player_Camera_GetFront()
{
	return g_CameraFront;
}

const DirectX::XMFLOAT3& Player_Camera_GetPosition()
{
	return g_CameraPosition;
}
