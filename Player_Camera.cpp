#include "Player_Camera.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include "key_logger.h"
#include "debug_text.h"
#include <sstream>
#include "shader_3d.h"
#include "shader_field.h"
#include "shader_billboard.h"
#include "direct3d.h"
#include "PlayerCharacter.h"
#include "shader3d_unlit.h"
using namespace DirectX;


namespace {
	DirectX::XMFLOAT3 g_CameraFront = { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 g_CameraPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 g_SmoothedTarget = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 g_ViewMatrix{};
	DirectX::XMFLOAT4X4 g_ProjectionMatrix{};
	DirectX::XMFLOAT4X4 g_CameraMatrix;
	bool g_HasCameraState = false;

	// --- 暗黑风格参数配置 ---
	const float CAM_HEIGHT = 14.0f;       // 相机高度 (Y)
	const float CAM_DISTANCE = -11.0f;    // 相机后退距离 (Z)
	const float CAMERA_FOLLOW_SHARPNESS = 8.0f; // 位置跟随响应速度，使用 dt 保持不同帧率下稳定
	const float TARGET_FOLLOW_SHARPNESS = 14.0f; // 注视目标响应速度，抑制移动/近战时的细小抖动
	const float CAMERA_TARGET_Y = 0.0f;   // 俯视角相机不跟随角色脚底高度的微小变化
	const float LOOK_AHEAD_Z = 1.0f;      // 保留原本略向前看的构图
	float g_ShakeIntensity = 0.0f;
	float RandomFloatCam(float min, float max) {
		return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (max - min));
	}

	float MakeFrameStableLerp(float sharpness, float deltaTime)
	{
		if (deltaTime <= 0.0f)
		{
			return 0.0f;
		}

		return std::clamp(
			1.0f - std::exp(-sharpness * deltaTime),
			0.0f,
			1.0f);
	}
}


void Player_Camera_Initialize()
{
	g_CameraFront = { 0.0f, 0.0f, 1.0f };
	g_CameraPosition = { 0.0f, CAM_HEIGHT, CAM_DISTANCE };
	g_SmoothedTarget = { 0.0f, CAMERA_TARGET_Y, 0.0f };
	g_ShakeIntensity = 0.0f;
	g_HasCameraState = false;
}

void Player_Camera_Finalize()
{
}

void Player_Camera_Update(double elapsed_time, const DirectX::XMFLOAT3& playerPos)
{
	const float dt = static_cast<float>(elapsed_time);

	if (g_ShakeIntensity > 0.0f) {
		g_ShakeIntensity -= dt * 3.0f; // 震动衰减速度，越大停得越快
		if (g_ShakeIntensity < 0.0f) g_ShakeIntensity = 0.0f;
	}

	XMVECTOR rawTarget = XMVectorSet(playerPos.x, CAMERA_TARGET_Y, playerPos.z, 0.0f);
	XMVECTOR offset = XMVectorSet(0.0f, CAM_HEIGHT, CAM_DISTANCE, 0.0f);

	if (!g_HasCameraState)
	{
		XMStoreFloat3(&g_SmoothedTarget, rawTarget);

		XMVECTOR initialPos = XMVectorAdd(rawTarget, offset);
		XMStoreFloat3(&g_CameraPosition, initialPos);
		g_HasCameraState = true;
	}

	XMVECTOR smoothedTarget = XMLoadFloat3(&g_SmoothedTarget);
	const float targetLerp = MakeFrameStableLerp(TARGET_FOLLOW_SHARPNESS, dt);
	smoothedTarget = XMVectorLerp(smoothedTarget, rawTarget, targetLerp);
	XMStoreFloat3(&g_SmoothedTarget, smoothedTarget);

	XMVECTOR idealPos = XMVectorAdd(smoothedTarget, offset);
	XMVECTOR currentPos = XMLoadFloat3(&g_CameraPosition);
	const float cameraLerp = MakeFrameStableLerp(CAMERA_FOLLOW_SHARPNESS, dt);
	XMVECTOR newPos = XMVectorLerp(currentPos, idealPos, cameraLerp);

	XMStoreFloat3(&g_CameraPosition, newPos);
	XMVECTOR lookAtPoint = XMVectorAdd(smoothedTarget, XMVectorSet(0.0f, 0.0f, LOOK_AHEAD_Z, 0.0f));
	XMVECTOR front = XMVector3Normalize(lookAtPoint - newPos);
	XMStoreFloat3(&g_CameraFront, front);

	// --- 新增：将震动偏移应用到注视点 ---
	if (g_ShakeIntensity > 0.0f) {
		float rx = RandomFloatCam(-1.0f, 1.0f) * g_ShakeIntensity;
		float ry = RandomFloatCam(-1.0f, 1.0f) * g_ShakeIntensity;
		float rz = RandomFloatCam(-1.0f, 1.0f) * g_ShakeIntensity;
		XMVECTOR shakeOffset = XMVectorSet(rx, ry, rz, 0.0f);
		lookAtPoint = XMVectorAdd(lookAtPoint, shakeOffset);
	}
	// -----------------------------------

	XMMATRIX mtxView = XMMatrixLookAtLH(newPos, lookAtPoint, { 0.0f, 1.0f, 0.0f });
	XMStoreFloat4x4(&g_ViewMatrix, mtxView);

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

void Player_Camera_AddShake(float intensity)
{
	// 累加震动强度，并设置一个上限防止震得太夸张
	g_ShakeIntensity += intensity;
	if (g_ShakeIntensity > 1.5f) {
		g_ShakeIntensity = 1.5f;
	}
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

	RECT clientRect;
	GetClientRect(Direct3D_GetWindowHandle(), &clientRect);
	float sw = static_cast<float>(clientRect.right - clientRect.left);
	float sh = static_cast<float>(clientRect.bottom - clientRect.top);

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
