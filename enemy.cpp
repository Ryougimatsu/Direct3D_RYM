#include "enemy.h"
#include "enemy_test.h"
using namespace DirectX;


namespace {
	const int MAX_ENEMY = 25;
	Enemy* g_Enemy[MAX_ENEMY]{};
	int g_EnemyCount = 0;
}


void Enemy::Update(double elapsed_time)
{
	m_pState->Update(elapsed_time);
}
void Enemy::Draw() const
{
	m_pState->Draw();
}
void Enemy::UpdateState()
{
	if (m_pNextState != m_pState)
	{
		delete m_pState;
		m_pState = m_pNextState;
	}

}
void Enemy::ChangeState(State* pNextState)
{
	m_pNextState = pNextState;
}

void Enemy_Initialize()
{
	g_EnemyCount = 0;
}

void Enemy_Finalize()
{
	for (int i = 0; i < g_EnemyCount; ++i)
	{
			delete g_Enemy[i];
			g_Enemy[i] = nullptr;
	}
}

void Enemy_Update(double elapsed_time)
{
	for (int i = 0; i < g_EnemyCount; ++i)
	{
		g_Enemy[i]->UpdateState();
	}
	for (int i = 0; i < g_EnemyCount; ++i)
	{
		g_Enemy[i]->Update(elapsed_time);
	}
	for (int i = 0; i < g_EnemyCount; ++i)
	{
		if (g_Enemy[i]->IsDestroyed())
		{
			delete g_Enemy[i];
			g_Enemy[i] = g_Enemy[g_EnemyCount - 1];
			g_Enemy[g_EnemyCount - 1] = nullptr;
			--g_EnemyCount;
			--i;
		}
	}
}

void Enemy_Draw()
{
	for (int i = 0; i < g_EnemyCount; ++i)
	{
		g_Enemy[i]->Draw();
	}
}

void Enemy_Create(const XMFLOAT3& position)
{
	g_Enemy[g_EnemyCount++] = new EnemyTest(position);
}
