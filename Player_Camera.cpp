#include "Player_Camera.h"
#include <DirectXMath.h>
#include "key_logger.h"
#include "debug_text.h"
#include <sstream>
#include "shader_3d.h"
#include "shader_field.h"
#include "direct3d.h"
#include "Player.h"
using namespace DirectX;


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
	XMVECTOR position = XMLoadFloat3(&Player_GetPosition());

	position = XMVectorMultiply(position, { 1.0f,0.0f,1.0f });

	XMVECTOR target = position;

	position = XMVectorAdd(position, { 0.0f,3.0f,-5.0f });


	XMVECTOR front = XMVector3Normalize(target - position);

	XMStoreFloat3(&g_CameraFront, front);
	XMStoreFloat3(&g_CameraPosition, position);

	XMMATRIX mtxView = XMMatrixLookAtLH(
		position, // 視点座標
		target, // 注視点座標
		{ 0.0f,1.0f,0.0f } // 上方向ベクトル
	);

	Shader_3D_SetViewMatrix(mtxView);
	Shader_field_SetViewMatrix(mtxView);

	float aspectRatio = static_cast<float>(Direct3D_GetBackBufferWidth()) / static_cast<float>(Direct3D_GetBackBufferHeight());
	float nearZ = 0.1f;
	float farZ = 1000.0f;

	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(
		1.0f,
		aspectRatio,
		nearZ,
		farZ
	);

	Shader_3D_SetProjectMatrix(mtxPerspective);
	Shader_field_SetProjectMatrix(mtxPerspective);
}

const DirectX::XMFLOAT3& Player_Camera_GetFront()
{
	return g_CameraFront;
}

const DirectX::XMFLOAT3& Player_Camera_GetPosition()
{
	return g_CameraPosition;
}
