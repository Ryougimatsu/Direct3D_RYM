#include "DebugCamera.h"
#include "keyboard.h"
#include "key_logger.h"
#include "mouse.h" 
#include "direct3d.h"
#include <algorithm>
using namespace DirectX;

namespace {
	XMFLOAT3 g_Position = { 0.0f, 0.0f, 0.0f }; // 相机位置
	XMFLOAT3 g_Rotation = { 0.0f, 0.0f, 0.0f };   // 相机旋转 (x:Pitch, y:Yaw, z:Roll)
	XMFLOAT4X4 g_ViewMatrix;
	XMFLOAT4X4 g_ProjectionMatrix;

	// 参数设置
	float g_MoveSpeed = 10.0f;     // 移动速度
	float g_MouseSensitivity = 0.1f; // 鼠标灵敏度

}
void DebugCamera_Initialize(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rot)
{
	g_Position = pos;
	g_Rotation = rot;

	// 初始化投影矩阵 (通常只做一次)
	float width = (float)Direct3D_GetBackBufferWidth();
	float height = (float)Direct3D_GetBackBufferHeight();
	// 45度视野, 0.1近平面, 1000.0远平面
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), width / height, 0.1f, 1000.0f);
	XMStoreFloat4x4(&g_ProjectionMatrix, proj);
}

void DebugCamera_Update(double elapsed_time)
{
	// ----------------------------------------
	// 1. 鼠标旋转 (按住右键时生效，或者根据你的需求一直生效)
	// ----------------------------------------
	// 假设 Mouse_GetVelocity() 返回鼠标每帧的位移 {x, y, z}
	// 如果没有 Velocity 函数，可以用 Mouse_GetPosition() - 上一帧位置
	Mouse_State mState;
	Mouse_GetState(&mState);
	if (mState.positionMode == MOUSE_POSITION_MODE_RELATIVE)
	{
		// 在 Relative 模式下，x 和 y 是增量 (Delta)
		float deltaX = (float)mState.x;
		float deltaY = (float)mState.y;

		// 更新角度
		g_Rotation.y += deltaX * g_MouseSensitivity;
		g_Rotation.x += deltaY * g_MouseSensitivity;

		// 限制俯仰角 (Pitch)，防止翻转
		g_Rotation.x = std::max(-89.0f, std::min(89.0f, g_Rotation.x));
	}

	// ----------------------------------------
	// 2. 键盘移动 (WASD + Shift加速)
	// ----------------------------------------
	float speed = g_MoveSpeed * (float)elapsed_time;
	if (KeyLogger_IsPressed(KK_LEFTSHIFT)) speed *= 3.0f; // 按 Shift 加速

	// 计算当前的“前”和“右”向量
	// 将旋转角度转换为弧度
	float yawR = XMConvertToRadians(g_Rotation.y);
	float pitchR = XMConvertToRadians(g_Rotation.x);

	// 基础向量
	XMVECTOR forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// 创建旋转矩阵
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(pitchR, yawR, 0.0f);

	// 变换方向向量
	forward = XMVector3TransformNormal(forward, rotationMatrix);
	right = XMVector3TransformNormal(right, rotationMatrix);
	// up 通常保持 (0,1,0) 以保持平稳，但在自由飞行模式下也可以随旋转变换

	XMVECTOR pos = XMLoadFloat3(&g_Position);

	if (KeyLogger_IsPressed(KK_W)) pos += forward * speed;
	if (KeyLogger_IsPressed(KK_S)) pos -= forward * speed;
	if (KeyLogger_IsPressed(KK_D)) pos += right * speed;
	if (KeyLogger_IsPressed(KK_A)) pos -= right * speed;
	if (KeyLogger_IsPressed(KK_Q)) pos += XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) * speed; // 垂直上升
	if (KeyLogger_IsPressed(KK_E)) pos -= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) * speed; // 垂直下降

	XMStoreFloat3(&g_Position, pos);

	// ----------------------------------------
	// 3. 计算 View Matrix
	// ----------------------------------------
	// 目标点 = 当前位置 + 前方向量
	XMVECTOR target = pos + forward;
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up); // Up 永远朝上
	XMStoreFloat4x4(&g_ViewMatrix, view);
}

DirectX::XMFLOAT4X4 DebugCamera_GetViewMatrix()
{
	return g_ViewMatrix;
}

DirectX::XMFLOAT4X4 DebugCamera_GetProjectionMatrix()
{
	return g_ProjectionMatrix;
}

DirectX::XMFLOAT3 DebugCamera_GetPosition()
{
	return g_Position;
}

void DebugCamera_SetPosition(DirectX::XMFLOAT3 pos)
{
	g_Position = pos;
}
