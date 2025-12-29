#include "Player_Camera.h"
#include <DirectXMath.h>
#include "key_logger.h"
#include "debug_text.h"
#include <sstream>
#include "shader_3d.h"
#include "shader_field.h"
#include "shader_billboard.h"
#include "direct3d.h"
#include "Player.h"
#include "shader3d_unlit.h"
using namespace DirectX;


namespace {
	DirectX::XMFLOAT3 g_CameraFront = { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 g_CameraPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 g_ViewMatrix{};
	DirectX::XMFLOAT4X4 g_ProjectionMatrix{};
	DirectX::XMFLOAT4X4 g_CameraMatrix;

	// --- 暗黑风格参数配置 ---
	const float CAM_HEIGHT = 14.0f;       // 相机高度 (Y)
	const float CAM_DISTANCE = -11.0f;    // 相机后退距离 (Z)
	const float CAM_LERP_SPEED = 0.1f;    // 跟随平滑度 (0.1 表示每帧向玩家靠近 10%)
}


void Player_Camera_Initialize()
{

}

void Player_Camera_Finalize()
{
}

void Player_Camera_Update(double elapsed_time, const DirectX::XMFLOAT3& playerPos)
{
	// 1. 计算理想的相机位置 (Target Position)
	// 这个偏移量决定了“暗黑”视角的角度
	XMVECTOR targetPlayer = XMLoadFloat3(&playerPos);
	XMVECTOR offset = { 0.0f, CAM_HEIGHT, CAM_DISTANCE };
	XMVECTOR idealPos = XMVectorAdd(targetPlayer, offset);

	// 2. 平滑跟随逻辑 (Lerp)
	// 让相机当前位置向理想位置平滑插值
	XMVECTOR currentPos = XMLoadFloat3(&g_CameraPosition);
	XMVECTOR newPos = XMVectorLerp(currentPos, idealPos, CAM_LERP_SPEED);

	// 3. 存储位置和方向
	XMStoreFloat3(&g_CameraPosition, newPos);
	XMVECTOR front = XMVector3Normalize(targetPlayer - newPos);
	XMStoreFloat3(&g_CameraFront, front);

	// 4. 生成观察矩阵
	// 注视点可以是玩家位置稍微往前半米（Diablo 常用技巧，让前方视野更大）
	XMVECTOR lookAtPoint = XMVectorAdd(targetPlayer, { 0.0f, 0.0f, 1.0f });
	XMMATRIX mtxView = XMMatrixLookAtLH(newPos, lookAtPoint, { 0.0f, 1.0f, 0.0f });
	XMStoreFloat4x4(&g_ViewMatrix, mtxView);

	// 5. 生成投影矩阵 (关键：较小的 FOV)
	float aspectRatio = static_cast<float>(Direct3D_GetBackBufferWidth()) / static_cast<float>(Direct3D_GetBackBufferHeight());


	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(
		0.5f,
		aspectRatio,
		0.1f,
		1000.0f
	);
	XMStoreFloat4x4(&g_ProjectionMatrix, mtxPerspective);
	XMMATRIX mtxCamera = XMMatrixInverse(nullptr, mtxView);
	XMStoreFloat4x4(&g_CameraMatrix, mtxCamera);
}

const DirectX::XMFLOAT3& Player_Camera_GetFront()
{
	return g_CameraFront;
}

const DirectX::XMFLOAT3& Player_Camera_GetPosition()
{
	return g_CameraPosition;
}

const DirectX::XMFLOAT4X4& Player_Camera_GetMatrix()
{
	return g_CameraMatrix;
}

const DirectX::XMFLOAT4X4& Player_Camera_GetViewMatrix()
{
	return g_ViewMatrix;
}

const DirectX::XMFLOAT4X4& Player_Camera_GetProjectionMatrix()
{
	return g_ProjectionMatrix;
}

XMVECTOR GetMouseWorldPos(const XMMATRIX& view, const XMMATRIX& proj) {
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(Direct3D_GetWindowHandle(), &pt);

	float sw = (float)Direct3D_GetBackBufferWidth();
	float sh = (float)Direct3D_GetBackBufferHeight();

	// 1. 转换到 NDC 空间
	float ndcX = (2.0f * pt.x) / sw - 1.0f;
	float ndcY = 1.0f - (2.0f * pt.y) / sh;

	// 2. 计算逆矩阵
	XMMATRIX invViewProj = XMMatrixInverse(nullptr, view * proj);

	// 3. 计算射线起点（近平面）和终点（远平面）
	XMVECTOR nearPoint = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
	XMVECTOR farPoint = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

	XMVECTOR worldNear = XMVector3TransformCoord(nearPoint, invViewProj);
	XMVECTOR worldFar = XMVector3TransformCoord(farPoint, invViewProj);

	// 4. 射线与地面 (Y=0) 求交
	XMVECTOR rayDir = XMVector3Normalize(worldFar - worldNear);
	float t = -XMVectorGetY(worldNear) / XMVectorGetY(rayDir);

	return XMVectorAdd(worldNear, XMVectorScale(rayDir, t));
}