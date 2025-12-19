#include "Player.h"
#include "model.h"
#include "key_logger.h"
#include "Light.h"
#include "camera.h"
#include "Character.h"
#include "Player_Camera.h"
#include "map.h"
#include "cube.h"
#include "bullet.h"
#include "Meshfield.h"
#include "direct3d.h"
using namespace DirectX;

namespace {
	XMFLOAT3 g_PlayerPosition = {};
	XMFLOAT3 g_PlayerFront = { 0.0f,0.0f,1.0f };
	XMFLOAT3 g_PlayerVelocity = {};
	MODEL* g_pPlayerModel = nullptr;
	bool g_IsJump = false;
	const float PLAYER_HEIGHT = 1.2f;
	const float PLAYER_HALF_WIDTH_X = 1.0f / 2.0f;
	const float PLAYER_HALF_WIDTH_Z = 1.0f / 2.0f;
	float g_PlayerHP = 100.0f;
	float g_PlayerMaxHP = 100.0f;
	float g_InvincibleTimer = 0.0f;
}

void Player_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3 front)
{
	g_PlayerPosition = position;
	g_PlayerVelocity = { 0.0f,0.0f,0.0f };
	XMStoreFloat3(&g_PlayerFront, XMVector3Normalize(XMLoadFloat3(&front)));
	g_pPlayerModel = ModelLoad("resource/Model/Player-T-Pose.fbx",0.01f);
}

void Player_Finalize()
{
	ModelRelease(g_pPlayerModel);
}

void Player_Update(double elapsed_time)
{
	//无敌时间计时器
	if (g_InvincibleTimer > 0.0f) {
		g_InvincibleTimer -= static_cast<float>(elapsed_time);
	}
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
	float groundHeight = MeshField_GetHeight(XMVectorGetX(position), XMVectorGetZ(position));
	float heightOffset = -2.0f;
	float finalY = groundHeight + heightOffset;
	if (XMVectorGetY(position) <= finalY)
	{

		if (XMVectorGetY(velocity) <= 0.0f)
		{
			position = XMVectorSetY(position, finalY);

		
			velocity *= { 1.0f, 0.0f, 1.0f };

		
			g_IsJump = false;
		}
	}
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
		XMFLOAT3 shoot_pos;

		
		XMVECTOR vPos = XMLoadFloat3(&g_PlayerPosition);
		XMVECTOR vFront = XMLoadFloat3(&g_PlayerFront);

	
		XMVECTOR vOffsetY = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		
		float forwardDist = 0.5f;

		
		XMVECTOR vShootPos = vPos + vOffsetY + (vFront * forwardDist);

		
		XMStoreFloat3(&shoot_pos, vShootPos);

		
		XMStoreFloat3(&b_velocity, vFront * 10.0f); 

	
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
		g_PlayerPosition.y,
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
float Player_GetHP() {
	return g_PlayerHP;
}

float Player_GetMaxHP() {
	return g_PlayerMaxHP;
}

void Player_Damage(float damage) {

	//无敌时间中不受伤
	if (g_InvincibleTimer > 0.0f) return;

	g_PlayerHP -= damage;
	if (g_PlayerHP < 0.0f) g_PlayerHP = 0.0f;
	//受到伤害后进入无敌时间
	g_InvincibleTimer = 1.0f;
}

void Player_Heal(float amount) {
	g_PlayerHP += amount;
	if (g_PlayerHP > g_PlayerMaxHP) g_PlayerHP = g_PlayerMaxHP;
}
