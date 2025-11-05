#include "Player.h"
#include "model.h"
#include "key_logger.h"
#include "Light.h"
#include "camera.h"
#include "Player_Camera.h"
#include "cube.h"
using namespace DirectX;

namespace {
	DirectX::XMFLOAT3 g_PlayerPosition = {};
	DirectX::XMFLOAT3 g_PlayerFront = { 0.0f,0.0f,1.0f };
	DirectX::XMFLOAT3 g_PlayerVelocity = {};
	MODEL* g_pPlayerModel = nullptr;
	bool g_IsJump = false;
}

void Player_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3 front)
{
	g_PlayerPosition = position;
	g_PlayerVelocity = { 0.0f,0.0f,0.0f };
	DirectX::XMStoreFloat3(&g_PlayerFront, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&front)));
	g_pPlayerModel = ModelLoad("resource/Model/test.fbx", 0.1f);
}

void Player_Finalize()
{
	ModelRelease(g_pPlayerModel);
}

void Player_Update(double elapsed_time)
{
	XMVECTOR position = XMLoadFloat3(&g_PlayerPosition);
	XMVECTOR velocity = XMLoadFloat3(&g_PlayerVelocity);

	//移動
	if (KeyLogger_IsTrigger(KK_SPACE) && !g_IsJump)
	{
		velocity += {0.0f, 8.0f, 0.0f };
		g_IsJump = true;
	}

	//重力
	XMVECTOR gravityDir = { 0.0f,-1.0f };
	velocity += gravityDir * 9.8f * 1.5f * static_cast<float>(elapsed_time);
	position += velocity * static_cast<float>(elapsed_time);

	//地面判定
	if (XMVectorGetY(position) < 1.0f)
	{
		position -= velocity * static_cast<float>(elapsed_time);
		velocity *= { 1.0f, 0.0f, 1.0f };
		g_IsJump = false;
	}

	XMVECTOR direction{};
	XMVECTOR front = XMLoadFloat3(&Player_Camera_GetFront()) * XMVECTOR { 1.0f, 0.0f, 1.0f };

	if (KeyLogger_IsPressed(KK_W))
	{
		direction += front;
	}
	if (KeyLogger_IsPressed(KK_S))
	{
		direction -= front;
	}


	if (KeyLogger_IsPressed(KK_A))
	{
		direction -= XMVector3Cross({ 0.0f,1.0f,0.0f }, front);

	}
	if (KeyLogger_IsPressed(KK_D))
	{
		direction += XMVector3Cross({ 0.0f,1.0f,0.0f }, front);
	}

	if (XMVectorGetX(XMVector3LengthSq(direction)) > 0.0f) {

		direction = XMVector3Normalize(direction);


		float dot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&g_PlayerFront), direction));

		float angle = acosf(dot);

		const float ROT_SPEED = XM_2PI * 2.0f * static_cast<float>(elapsed_time);

		if (angle < ROT_SPEED)
		{
			front = direction;
		}
		else
		{//回転行列を使って徐々に向きを変える

			XMMATRIX r = XMMatrixIdentity();

			if (XMVectorGetY(XMVector3Cross(XMLoadFloat3(&g_PlayerFront), direction)) < 0.0f)
			{
				r = XMMatrixRotationY(-ROT_SPEED);
			}
			else
			{
				r = XMMatrixRotationY(ROT_SPEED);
			}

			front = XMVector3TransformNormal(XMLoadFloat3(&g_PlayerFront), r);
		}

		velocity += front * static_cast<float>(2000.0 / 50.0 * elapsed_time);

		XMStoreFloat3(&g_PlayerFront, front);
	}

	velocity += -velocity * static_cast<float>(4.0f * elapsed_time);
	position += velocity * static_cast<float>(elapsed_time);


	XMStoreFloat3(&g_PlayerPosition, position);
	XMStoreFloat3(&g_PlayerVelocity, velocity);
	
	AABB player = Player_GetAABB();
	AABB cube = Cube_CreateAABB({ 3.0f, 0.5f, 2.0f });

	if (Collision_IsOverAABB(player, cube)) {
		position -= velocity * (float)elapsed_time;
		velocity = { 0.0f, 0.0f, 0.0f };
	}

}

void Player_Draw()
{
	Light_SetSpecularWorld(Camera_GetPosition(), 10.0f, { 0.1f,0.1f,0.1f,0.1f });//镜面反射光

	float angle = -atan2f(g_PlayerFront.z, g_PlayerFront.x) + XMConvertToRadians(270);

	XMMATRIX r = XMMatrixRotationY(angle);

	DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(
		g_PlayerPosition.x,
		g_PlayerPosition.y,
		g_PlayerPosition.z
	);
	ModelDraw(g_pPlayerModel, world);
}

AABB Player_GetAABB()
{
	return{
		{ g_PlayerPosition.x - 1.0f,
		  g_PlayerPosition.y,
		  g_PlayerPosition.z - 1.0f,
		},
		{ g_PlayerPosition.x + 1.0f,
		  g_PlayerPosition.y + 2.0f,
		  g_PlayerPosition.z + 1.0f
		}
	};
}


const DirectX::XMFLOAT3& Player_GetPosition()
{
	return g_PlayerPosition;
}

const DirectX::XMFLOAT3& Player_GetFront()
{
	return g_PlayerFront;
}