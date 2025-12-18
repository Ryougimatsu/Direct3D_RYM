#include "enemy_test.h"
#include "collision.h"
#include "Player.h"
using namespace DirectX;
#include "cube.h"
#include "shader_3d.h"
#include "Meshfield.h"
#include "Player.h"

void EnemyTest::EnemyTest_StatePatrol::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;
	m_pOwner->m_position.x = m_PointX +sinf(m_AccumulatedTime);
	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);

	if (Collision_IsOverlapSphere({ m_pOwner->m_position,m_pOwner->m_DetectionAngle}, Player_GetPosition()))
	{
		m_pOwner->ChangeState(new EnemyTest_StateChase(m_pOwner));
	}
}

void EnemyTest::EnemyTest_StatePatrol::Draw() const
{

	Cube_Draw(m_pOwner->m_TexID, XMMatrixTranslation(m_pOwner->m_position.x, m_pOwner->m_position.y, m_pOwner->m_position.z));
}

void EnemyTest::EnemyTest_StateChase::Update(double elapsed_time)
{

	XMFLOAT3 playerPos = Player_GetPosition();
	XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);
	XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position);

	XMVECTOR toPlayer = vPlayerPos - vEnemyPos;
	toPlayer = XMVectorSetY(toPlayer, 0.0f);
	//追踪速度0.5f
	if (XMVectorGetX(XMVector3LengthSq(toPlayer)) > 0.0001f)
	{
		toPlayer = XMVector3Normalize(toPlayer);

		float speed = 2.5f; // 追踪速度
		XMVECTOR vNewPos = vEnemyPos + toPlayer * speed * (float)elapsed_time;

		XMStoreFloat3(&m_pOwner->m_position, vNewPos);
	}

	//放弃追踪
	if (!Collision_IsOverlapSphere({ m_pOwner->m_position,m_pOwner->m_DetectionAngle }, Player_GetPosition()))
	{
		m_AccumulatedTime += elapsed_time;

		if(m_AccumulatedTime >= 3.0)
		{
			m_pOwner->ChangeState(new EnemyTest_StatePatrol(m_pOwner));
		}
	}
	else
	{
		m_AccumulatedTime = 0.0;
	}
	//攻击判定：检测敌人身体是否碰到了玩家
	Sphere enemySphere = { m_pOwner->GetPosition(), 0.5f };

	// 获取玩家位置，假设玩家半径也是 0.5f (可以做的更精确点，用 Player_GetAABB)
	Sphere playerSphere = { Player_GetPosition(), 1.0f };

	if (Collision_IsOverlapSphere(enemySphere, playerSphere))
	{
		// 碰到玩家了！造成 10 点伤害
		Player_Damage(10.0f);

		// 此时 Player_Damage 内部会自动处理无敌时间，
		// 所以即使这里每一帧都调用，实际上 1 秒只会扣一次血。
	}
}

void EnemyTest::EnemyTest_StateChase::Draw() const
{
	Cube_Draw(m_pOwner->m_TexID, XMMatrixTranslation(m_pOwner->m_position.x, m_pOwner->m_position.y, m_pOwner->m_position.z));
}
