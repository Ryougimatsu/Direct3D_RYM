#include "Player.h"
#include "model.h"
#include "key_logger.h"
#include "Light.h"
#include "camera.h"
#include "Player_Camera.h"
#include "map.h"
#include "cube.h"
#include "bullet.h"
using namespace DirectX;

namespace {
	XMFLOAT3 g_PlayerPosition = {};
	XMFLOAT3 g_PlayerFront = { 0.0f,0.0f,1.0f };
	XMFLOAT3 g_PlayerVelocity = {};
	MODEL* g_pPlayerModel = nullptr;
	bool g_IsJump = false;
	const float PLAYER_HEIGHT = 1.2f;
	const float PLAYER_HALF_WIDTH_X = 1.0f / 2.0f; // 假设测量后得到
	const float PLAYER_HALF_WIDTH_Z = 1.0f / 2.0f; // 假设X和Z是对称的
}

void Player_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3 front)
{
	g_PlayerPosition = position;
	g_PlayerVelocity = { 0.0f,0.0f,0.0f };
	XMStoreFloat3(&g_PlayerFront, XMVector3Normalize(XMLoadFloat3(&front)));
	g_pPlayerModel = ModelLoad("resource/Model/test.fbx", 0.05f);
}

void Player_Finalize()
{
	ModelRelease(g_pPlayerModel);
}

void Player_Update(double elapsed_time)
{
	XMVECTOR position = XMLoadFloat3(&g_PlayerPosition);
	XMVECTOR velocity = XMLoadFloat3(&g_PlayerVelocity);
	XMVECTOR gravityVelocity = XMVectorZero();

	//移動
	//ジャンプ
	if (KeyLogger_IsTrigger(KK_SPACE) && !g_IsJump)
	{
		velocity += {0.0f, 15.0f, 0.0f };
		g_IsJump = true;
	}

	//重力
	XMVECTOR gravityDir = { 0.0f, -1.0f };
	velocity += gravityDir * 9.8f * 1.5f * static_cast<float>(elapsed_time);

	gravityVelocity = velocity * static_cast<float>(elapsed_time);
	position += gravityVelocity;

	for (int i = 0; i < Map_GetObjectsCount(); i++)
	{

		AABB player = Player_ConvertPositionToAABB(position);
		AABB obj = Map_GetObject(i)->Aabb;

		Hit hit = Collision_IsHitAABB(obj, player);

		if (hit.isHit)
		{
			if (hit.normal.y > 0.0f)
			{
				position = XMVectorSetY(position, obj.max.y);
				velocity *= { 1.0f, 0.0f, 1.0f };
				g_IsJump = false;
			}
		}

	}
	//地面に到達したら止まる
	/*if (XMVectorGetY(position) <= 0.0f)
	{
		position = XMVectorSetY(position, 0.0f);
		gravityVelocity = XMVectorZero();
		velocity *= { 1.0f, 0.0f, 1.0f };
		g_IsJump = false;
	}*/

	XMVECTOR moveDir = XMVectorZero();

	XMFLOAT3 camFront = Player_Camera_GetFront();

	XMVECTOR front = XMVector3Normalize(XMVectorSet(camFront.x, 0.0f, camFront.z, 0.0f));
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), front));

	if (KeyLogger_IsPressed(KK_W)) moveDir += front;
	if (KeyLogger_IsPressed(KK_S)) moveDir -= front;
	if (KeyLogger_IsPressed(KK_D)) moveDir += right;
	if (KeyLogger_IsPressed(KK_A)) moveDir -= right;

	if (XMVectorGetX(XMVector3LengthSq(moveDir)) > 0.0f) {

		moveDir = XMVector3Normalize(moveDir);

		float dot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&g_PlayerFront), moveDir));

		float angle = acosf(dot);

		const float ROT_SPEED = XM_2PI * 2.0f * static_cast<float>(elapsed_time);

		if (angle < ROT_SPEED)
		{
			front = moveDir;
		}
		else
		{//回転行列を使って徐々に向きを変える

			XMMATRIX r = XMMatrixIdentity();

			if (XMVectorGetY(XMVector3Cross(XMLoadFloat3(&g_PlayerFront), moveDir)) < 0.0f)
			{
				r = XMMatrixRotationY(-ROT_SPEED);
			}
			else
			{
				r = XMMatrixRotationY(ROT_SPEED);
			}

			front = XMVector3TransformNormal(XMLoadFloat3(&g_PlayerFront), r);
		}

		velocity += front * static_cast<float>(2000.0 / 90.0 * elapsed_time);

		XMStoreFloat3(&g_PlayerFront, front);
	}

	velocity += -velocity * static_cast<float>(4.0f * elapsed_time);
	position += velocity * static_cast<float>(elapsed_time);

	for (int i = 0; i < Map_GetObjectsCount(); i++)
	{

		AABB player = Player_ConvertPositionToAABB(position);
		AABB obj = Map_GetObject(i)->Aabb;

		Hit hit = Collision_IsHitAABB(obj, player);

		if (hit.isHit)
		{
			if (hit.normal.x > 0.0f)
			{
				position = XMVectorSetX(position, obj.max.x + PLAYER_HALF_WIDTH_X);
				velocity *= { 0.0f, 1.0f, 1.0f };
			}
			else if (hit.normal.x < 0.0f)
			{
				position = XMVectorSetX(position, obj.min.x - PLAYER_HALF_WIDTH_X);
				velocity *= { 0.0f, 1.0f, 1.0f };
			}
			else if (hit.normal.y < 0.0f)
			{
				position = XMVectorSetY(position, obj.min.y - PLAYER_HEIGHT);
				velocity *= { 1.0f, 0.0f, 1.0f };
			}
			else if (hit.normal.z > 0.0f)
			{
				position = XMVectorSetZ(position, obj.max.z + PLAYER_HALF_WIDTH_Z); 
				velocity *= { 1.0f, 1.0f, 0.0f };
			}
			else if (hit.normal.z < 0.0f)
			{
				position = XMVectorSetZ(position, obj.min.z - PLAYER_HALF_WIDTH_Z); 
				velocity *= { 1.0f, 1.0f, 0.0f };
			}
		}
	}

	XMStoreFloat3(&g_PlayerPosition, position);
	XMStoreFloat3(&g_PlayerVelocity, velocity);
	//BULLET発射
	if (KeyLogger_IsTrigger(KK_F))
	{
		XMFLOAT3 b_velocity;
		XMFLOAT3 shoot_pos; // 不需要先等于 g_PlayerPosition，后面会覆盖

		// 1. 加载向量
		XMVECTOR vPos = XMLoadFloat3(&g_PlayerPosition);
		XMVECTOR vFront = XMLoadFloat3(&g_PlayerFront);

		// 2. 计算发射位置
		// OffsetY: 向上偏移 1.0f (大约在胸口或枪口高度)
		XMVECTOR vOffsetY = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		// OffsetForward: 向角色前方偏移 0.5f (避免子弹生成在身体内部)
		float forwardDist = 0.5f;

		// 最终位置 = 玩家位置 + 高度 + (朝向 * 距离)
		XMVECTOR vShootPos = vPos + vOffsetY + (vFront * forwardDist);

		// 3. 存回 float3
		XMStoreFloat3(&shoot_pos, vShootPos);

		// 4. 计算速度 (朝向 * 速度值)
		XMStoreFloat3(&b_velocity, vFront * 10.0f); // 假设速度是 20

		// 5. 创建子弹
		Bullet_Create(shoot_pos, b_velocity);
	}
}

void Player_Draw()
{
	Light_SetSpecularWorld(Player_Camera_GetPosition(), 4.0f, { 0.3f,0.25f,0.2f,1.0f });

	float angle = -atan2f(g_PlayerFront.z, g_PlayerFront.x) + XMConvertToRadians(270);

	XMMATRIX r = XMMatrixRotationY(angle);

	XMMATRIX t = XMMatrixTranslation(
		g_PlayerPosition.x,
		g_PlayerPosition.y + (PLAYER_HEIGHT / 2.0f),
		g_PlayerPosition.z);

	XMMATRIX world = r * t;

	ModelDraw(g_pPlayerModel, world);
}

AABB Player_GetAABB()
{
	return { {g_PlayerPosition.x - PLAYER_HALF_WIDTH_X,  g_PlayerPosition.y, g_PlayerPosition.z - PLAYER_HALF_WIDTH_Z },
		{g_PlayerPosition.x + PLAYER_HALF_WIDTH_X, g_PlayerPosition.y + PLAYER_HEIGHT, g_PlayerPosition.z + PLAYER_HALF_WIDTH_Z}
	};
}

AABB Player_ConvertPositionToAABB(const DirectX::XMVECTOR& position)
{
	AABB aabb;
	XMStoreFloat3(&aabb.min, position - XMVECTOR{ PLAYER_HALF_WIDTH_X, 0.0f, PLAYER_HALF_WIDTH_Z });
	XMStoreFloat3(&aabb.max, position + XMVECTOR{ PLAYER_HALF_WIDTH_X, PLAYER_HEIGHT, PLAYER_HALF_WIDTH_Z });
	return aabb;
}


const DirectX::XMFLOAT3& Player_GetPosition()
{
	return g_PlayerPosition;
}

const DirectX::XMFLOAT3& Player_GetFront()
{
	return g_PlayerFront;
}