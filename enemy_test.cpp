#include "enemy_test.h"
#include "collision.h"
#include "Player.h"
#include "cube.h"
#include "shader_3d.h"
#include "Meshfield.h"
#include <cmath> 
#include <cstdlib>
#include <ctime>
using namespace DirectX;

MODEL* EnemyTest::g_pEnemyModel = nullptr;

// ========================================================
// 状态机逻辑实现
// ========================================================

EnemyTest::EnemyTest_StatePatrol::EnemyTest_StatePatrol(EnemyTest* pOwner)
	: m_pOwner(pOwner)
	, m_PointX(pOwner->m_position.x) // 访问子类自己的 m_position
{
}

void EnemyTest::EnemyTest_StatePatrol::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;

	m_pOwner->m_position.x = m_PointX + sinf((float)m_AccumulatedTime) * 2.0f; // * 2.0f 增加一点巡逻范围

	// 贴合地面高度
	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);

	// 索敌判定
	if (Collision_IsOverlapSphere({ m_pOwner->m_position, m_pOwner->m_DetectionAngle }, Player_GetPosition()))
	{
		m_pOwner->ChangeState(new EnemyTest_StateChase(m_pOwner));
	}
}

void EnemyTest::EnemyTest_StatePatrol::Draw() const
{

}
// ========================================================
// 状态机 - 追逐
// ========================================================
EnemyTest::EnemyTest_StateChase::EnemyTest_StateChase(EnemyTest* pOwner)
	: m_pOwner(pOwner)
{
}


void EnemyTest::EnemyTest_StateChase::Update(double elapsed_time)
{
	XMFLOAT3 playerPos = Player_GetPosition();
	XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);
	XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position); // [修复 3] m_Position

	XMVECTOR toPlayer = vPlayerPos - vEnemyPos;
	toPlayer = XMVectorSetY(toPlayer, 0.0f);

	// 追踪移动
	if (XMVectorGetX(XMVector3LengthSq(toPlayer)) > 0.0001f)
	{
		toPlayer = XMVector3Normalize(toPlayer);

		float speed = 2.5f;
		XMVECTOR vNewPos = vEnemyPos + toPlayer * speed * (float)elapsed_time;

		XMStoreFloat3(&m_pOwner->m_position, vNewPos);

		m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);


		float angle = atan2f(XMVectorGetX(toPlayer), XMVectorGetZ(toPlayer));
		m_pOwner->SetRotationY(angle);
	}

	// 放弃追踪逻辑
	if (!Collision_IsOverlapSphere({ m_pOwner->m_position, m_pOwner->m_DetectionAngle * 1.5f }, Player_GetPosition())) 
	{
		m_AccumulatedTime += elapsed_time;

		if (m_AccumulatedTime >= 3.0)
		{
			m_pOwner->ChangeState(new EnemyTest_StatePatrol(m_pOwner));

			return;
		}
	}
	else
	{
		m_AccumulatedTime = 0.0;
	}

	// 攻击判定
	// 使用 GetAABB 获取模型精确碰撞盒
	AABB enemyAABB = m_pOwner->GetAABB();
	AABB playerAABB = Player_GetAABB();

	if (Collision_IsHitAABB(enemyAABB, playerAABB).isHit)
	{
		Player_Damage(10.0f);
	}
}
void EnemyTest::EnemyTest_StateChase::Draw() const
{

}

EnemyTest::EnemyTest(const DirectX::XMFLOAT3& position)
	: m_position(position) 
{
	m_HP = 30.0f; // 设置初始血量
	m_DetectionAngle = 5.0f; // 设置索敌半径

	// 初始化旋转
	float randomAngle = ((float)rand() / RAND_MAX) * DirectX::XM_2PI;
	m_Rotation = { 0.0f, randomAngle, 0.0f };

	// 设置初始状态为巡逻
	ChangeState(new EnemyTest_StatePatrol(this));
}

EnemyTest::~EnemyTest()
{

}


void EnemyTest::LoadAssets()
{
	if (g_pEnemyModel == nullptr)
	{
		// 这里的路径确保正确
		g_pEnemyModel = ModelLoad("resource/Model/Enemy-T-Pose.fbx", 0.01f);
	}
}

void EnemyTest::UnloadAssets()
{
	if (g_pEnemyModel != nullptr)
	{
		ModelRelease(g_pEnemyModel);
		g_pEnemyModel = nullptr;
	}
}

AABB EnemyTest::GetAABB()
{
	if (g_pEnemyModel != nullptr)
	{
		// 1. 先获取模型精确的原始盒子
		AABB aabb = ModelGetAABB(g_pEnemyModel, m_position);

		// 2. 定义一个适当的扩大值
		float padding = 0.5f;

		// 3. 手动扩大盒子
		aabb.min.x -= padding;
		aabb.min.y -= padding; 
		aabb.min.z -= padding;

		aabb.max.x += padding;
		aabb.max.y += padding + 1.0f;
		aabb.max.z += padding;

		return aabb;
	}

	
	return {
		{m_position.x - 1.0f, m_position.y, m_position.z - 1.0f},
		{m_position.x + 1.0f, m_position.y + 2.0f, m_position.z + 1.0f}
	};
}

void EnemyTest::Update(double elapsed_time)
{
	Enemy::Update(elapsed_time);
}

void EnemyTest::Draw() const
{

	if (g_pEnemyModel == nullptr) return;

	
	XMMATRIX S = XMMatrixScaling(1.0f, 1.0f, 1.0f);

	XMMATRIX R = XMMatrixRotationY(m_Rotation.y + XM_PI);

	XMMATRIX T = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

	XMMATRIX World = S * R * T;


	ModelDraw(g_pEnemyModel, World);


}


