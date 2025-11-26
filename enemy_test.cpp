#include "enemy_test.h"
#include "collision.h"
#include "Player.h"
using namespace DirectX;

void EnemyTest::EnemyTest_StatePatrol::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;
	m_pOwner->m_position.x = cosf(m_AccumulatedTime);

	if (Collision_IsOverlapSphere({ m_pOwner->m_position,m_pOwner->m_DetectionAngle}, Player_GetPosition()));
	{
		m_pOwner->ChangeState(new EnemyTest_StateChase(m_pOwner));
	}
}

void EnemyTest::EnemyTest_StatePatrol::Draw() const
{

}

void EnemyTest::EnemyTest_StateChase::Update(double elapsed_time)
{
	

	XMVECTOR toPlayer = XMLoadFloat3(&Player_GetPosition()) - XMLoadFloat3(&m_pOwner->m_position);
	toPlayer = XMVector3Normalize(toPlayer);
	//追踪速度0.5f
	XMVECTOR position = XMLoadFloat3(&m_pOwner->m_position) + toPlayer * 0.5f * elapsed_time;
	//放弃追踪
	if (!Collision_IsOverlapSphere({ m_pOwner->m_position,m_pOwner->m_DetectionAngle }, Player_GetPosition()));
	{
		m_AccumulatedTime += elapsed_time;

		if(m_AccumulatedTime >= 3.0)
		{
			m_pOwner->ChangeState(new EnemyTest_StatePatrol(m_pOwner));
		}
	}
}

void EnemyTest::EnemyTest_StateChase::Draw() const
{

}
