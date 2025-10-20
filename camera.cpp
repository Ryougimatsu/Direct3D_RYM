#include "camera.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "shader_3d.h"
#include "key_logger.h"
#include "debug_text.h"
#include <sstream>      
#include <iomanip>
#include <memory>


static XMFLOAT3 g_CameraPosition = { 0.0f, 0.0f, -5.0f };
static XMFLOAT3 g_CameraFront= { 0.0f, 0.0f, 1.0f };
static XMFLOAT3 g_CameraUp = { 0.0f, 1.0f,0.0f };
static XMFLOAT3 g_CameraRight = { 1.0f, 0.0f, 0.0f };
static constexpr float CAMERA_MOVE_SPEED = 4.5f;
static constexpr float CAMERA_UP_SPEED = 0.5f;
static constexpr float CAMERA_ROTATION_SPEED = XMConvertToRadians(30);
static XMFLOAT4X4 g_CameraMatrix = {};
static XMFLOAT4X4 g_PerspectiveMatrix = {};

static hal::DebugText* g_DebugText = nullptr;


void Camera_Initialize()
{
	g_CameraPosition = { 0.0f, 0.0f, -5.0f };

	g_CameraFront = { 0.0f, 0.0f, 1.0f };

	g_CameraUp = { 0.0f, 1.0f,0.0f };

	g_CameraRight = { 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&g_CameraMatrix, XMMatrixIdentity());

	XMStoreFloat4x4(&g_PerspectiveMatrix, XMMatrixIdentity());

#if defined(_DEBUG) || defined(DEBUG)

	g_DebugText = new hal::DebugText(Direct3D_GetDevice(), Direct3D_GetDeviceContext(),
		L"resource/texture/consolab_ascii_512.png",
		Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight(),
		0.0f, 25.0f,
		0, 0,
		0.0f, 16.0f
	);

#endif
}

void Camera_Initialize(const DirectX::XMFLOAT3& Position,
	const DirectX::XMFLOAT3& Front,
	const DirectX::XMFLOAT3& Right,
	const DirectX::XMFLOAT3& Up)
{
	Camera_Initialize();
	g_CameraPosition = Position;
	XMStoreFloat3(&g_CameraFront, XMVector3Normalize(XMLoadFloat3(&Front)));
	XMStoreFloat3(&g_CameraRight, XMVector3Normalize(XMLoadFloat3(&Right)));
	XMStoreFloat3(&g_CameraUp, XMVector3Normalize(XMLoadFloat3(&Up)));
}
void Camera_Finalize()
{
	delete g_DebugText;
}

void Camera_Update(double elapsed_time)
{
	XMVECTOR Front = XMLoadFloat3(&g_CameraFront);
	XMVECTOR Right = XMLoadFloat3(&g_CameraRight);
	XMVECTOR Up = XMLoadFloat3(&g_CameraUp);
	XMVECTOR Position = XMLoadFloat3(&g_CameraPosition);
	


	//向下
	if (KeyLogger_IsPressed(KK_DOWN))
	{
		XMMATRIX Rotation = XMMatrixRotationAxis(Right, CAMERA_UP_SPEED * elapsed_time);
		Front = XMVector3TransformNormal(Front, Rotation);
		Front = XMVector3Normalize(Front);
		Up = XMVector3Cross(Front, Right);
	}

	if (KeyLogger_IsPressed(KK_UP))
	{
		XMMATRIX Rotation = XMMatrixRotationAxis(Right, -CAMERA_UP_SPEED * elapsed_time);
		Front = XMVector3TransformNormal(Front, Rotation);
		Front = XMVector3Normalize(Front);
		Up = XMVector3Cross(Front, Right);
	}

	if(KeyLogger_IsPressed(KK_RIGHT)) {
		// XMMATRIX rotation = XMMatrixRotationAxis(up, CAMERA_ROTATION_SPEED * elapsed_time);
		XMMATRIX Rotation = XMMatrixRotationY(CAMERA_ROTATION_SPEED * elapsed_time); //
		Up = XMVector3Normalize(XMVector3TransformNormal(Up, Rotation)); //
		Front = XMVector3TransformNormal(Front, Rotation);
		Front = XMVector3Normalize(Front);
		Right = XMVector3Cross(Up, Front);
	}

	if (KeyLogger_IsPressed(KK_LEFT)) {
		// XMMATRIX rotation = XMMatrixRotationAxis(up, -CAMERA_ROTATION_SPEED * elapsed_time);
		XMMATRIX Rotation = XMMatrixRotationY(-CAMERA_ROTATION_SPEED * elapsed_time); //
		Up = XMVector3Normalize(XMVector3TransformNormal(Up, Rotation)); //
		Front = XMVector3TransformNormal(Front, Rotation);
		Front = XMVector3Normalize(Front);
		Right = XMVector3Cross(Up, Front);
	}


	if (KeyLogger_IsPressed(KK_W))
	{
		Position += Front * CAMERA_MOVE_SPEED * elapsed_time;
		//Position += XMVector3Normalize(Front * XMVECTOR{ 1.0f, 0.0f, 1.0f }) * CAMERA_MOVE_SPEED * elapsed_time;
	}

	if (KeyLogger_IsPressed(KK_A))
	{
		Position += -Right * CAMERA_MOVE_SPEED * elapsed_time;
	}

	if (KeyLogger_IsPressed(KK_S))
	{
		Position += -Front * CAMERA_MOVE_SPEED * elapsed_time;
	}

	if (KeyLogger_IsPressed(KK_D))
	{
		Position += Right * CAMERA_MOVE_SPEED * elapsed_time;
	}
	//向上
	if (KeyLogger_IsPressed(KK_J))
	{
		Position += Up * CAMERA_MOVE_SPEED * elapsed_time;
	}

	if (KeyLogger_IsPressed(KK_K))
	{
		Position += -Up * CAMERA_MOVE_SPEED * elapsed_time;
	}

	//结果保存
	XMStoreFloat3(&g_CameraPosition, Position);
	XMStoreFloat3(&g_CameraFront, Front);
	XMStoreFloat3(&g_CameraUp, Up);
	XMStoreFloat3(&g_CameraRight, Right);

	XMMATRIX mtxView = XMMatrixLookAtLH(
		Position,// 視点座標
		Position + Front, // 注視点座標
		Up // 上方向ベクトル
	);
	
	XMStoreFloat4x4(&g_CameraMatrix, mtxView);
	Shader_3D_SetViewMatrix(mtxView);

	constexpr float fovAngleY = XMConvertToRadians(60.0f);
	float aspectRatio = static_cast<float>(Direct3D_GetBackBufferWidth()) / static_cast<float>(Direct3D_GetBackBufferHeight());
	float nearZ = 0.1f;
	float farZ = 100.0f;

	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(
		fovAngleY,
		aspectRatio,
		nearZ,
		farZ
	);

	XMStoreFloat4x4(&g_PerspectiveMatrix, mtxPerspective);
	Shader_3D_SetProjectMatrix(mtxPerspective);
}

const DirectX::XMFLOAT4X4& Camera_GetMatrix()
{
	return g_CameraMatrix;
}

const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix()
{
	return g_PerspectiveMatrix;
}

const DirectX::XMFLOAT3& Camera_GetPosition()
{
	return g_CameraPosition;
}

const DirectX::XMFLOAT3& Camera_GetFront()
{
	return g_CameraFront;
}

const DirectX::XMFLOAT3& Camera_GetUp()
{
	return g_CameraUp;
}

const DirectX::XMFLOAT3& Camera_GetRight()
{
	return g_CameraRight;
}

void Debug_Draw()
{
#if defined(DEBUG) || defined(_DEBUG)
	// 1. 通过 Getter 函数从 camera.cpp 获取最新的摄像机数据
	const DirectX::XMFLOAT3& pos = Camera_GetPosition();
	const DirectX::XMFLOAT3& front = Camera_GetFront();
	const DirectX::XMFLOAT3& up = Camera_GetUp();
	const DirectX::XMFLOAT3& right = Camera_GetRight();

	std::stringstream ss;
	// 设置浮点数精度，让输出更整齐
	ss << std::fixed << std::setprecision(2);

	// 3. 将各个向量信息添加到 stringstream
	ss << "Cam Pos  : [" << pos.x << ", " << pos.y << ", " << pos.z << "]" << std::endl;
	ss << "Cam Front: [" << front.x << ", " << front.y << ", " << front.z << "]" << std::endl;
	ss << "Cam Up   : [" << up.x << ", " << up.y << ", " << up.z << "]" << std::endl;
	ss << "Cam Right: [" << right.x << ", " << right.y << ", " << right.z << "]" << std::endl;

	// 4. 设置文本、绘制并清空，与你的示例代码保持一致
	g_DebugText ->SetText(ss.str().c_str());
	g_DebugText ->Draw();
	g_DebugText ->Clear();
#endif
}
