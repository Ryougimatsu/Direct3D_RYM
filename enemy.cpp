#include "enemy.h"
#include "enemy_test.h"
#include "PlayerCharacter.h"
#include <vector>
#include <cstdlib>     
#include <ctime>
#include "SkinningShader.h"
using namespace DirectX;


namespace {
	std::vector<Enemy*> g_Enemies;

	const int MAX_ENEMIES_ON_SCREEN = 10; // 场上最多同时存在多少个敌人
	const float SPAWN_INTERVAL = 3.0f;    // 每几秒生成一只
	float g_SpawnTimer = 0.0f;            // 计时器

	const float MAP_RANGE = 20.0f;        // 地图大小 (假设是 -20 到 20)
	const float SAFE_DISTANCE = 5.0f;     // 安全距离 (玩家周围 5 米内不刷怪)

	int g_EnemyCount = 0;
	// -- - 辅助函数：生成随机浮点数-- -
		float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (max - min));
	}
}


void Enemy::Update(double elapsed_time)
{
	if (m_pState)
		m_pState->Update(elapsed_time);
}
void Enemy::Draw(DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj) const
{
	SkinningShader_3D_Begin();
	for (auto* e : g_Enemies) {
		e->Draw(view, proj);
	}
}
void Enemy::ChangeState(State* pNextState)
{
	if (m_pState) {
		delete m_pState; // 删除旧状态
	}
	m_pState = pNextState; // 切换新状态
}

void Enemy_Initialize()
{
	srand(static_cast<unsigned int>(time(nullptr)));
	EnemyTest::LoadAssets();

	for (auto* e : g_Enemies) delete e;
	g_Enemies.clear();

	Enemy_Create({ 5.0f, 0.0f, 5.0f });

	g_EnemyCount = 0;
}

void Enemy_Update(double elapsed_time)
{
	// 1. 更新现有敌人
	for (auto it = g_Enemies.begin(); it != g_Enemies.end(); )
	{
		(*it)->Update(elapsed_time);

		// 如果敌人死了 (IsDestroyed 返回 true)
		if ((*it)->IsDestroyed())
		{
			delete (*it);
			it = g_Enemies.erase(it); // 从列表中移除
		}
		else
		{
			++it;
		}
	}

	// ==========================================
	// [新增] 随机生成逻辑
	// ==========================================

	// 只有当敌人数量未达上限时，才开始计时
	if (g_Enemies.size() < MAX_ENEMIES_ON_SCREEN)
	{
		g_SpawnTimer += static_cast<float>(elapsed_time);

		if (g_SpawnTimer >= SPAWN_INTERVAL)
		{
			// 重置计时器
			g_SpawnTimer = 0.0f;

			// --- 计算随机位置 ---
			XMFLOAT3 spawnPos;
			XMFLOAT3 playerPos = Player_GetPosition();
			bool validPos = false;
			int attempts = 0;

			// 尝试 10 次寻找一个合适的位置 (防止一直找不到死循环)
			while (!validPos && attempts < 10)
			{
				// 1. 在 -20 ~ 20 范围内随机
				spawnPos.x = RandomFloat(-MAP_RANGE, MAP_RANGE);
				spawnPos.z = RandomFloat(-MAP_RANGE, MAP_RANGE);
				spawnPos.y = 0.0f; // 地面高度

				// 2. 检查距离玩家是否太近
				float dx = spawnPos.x - playerPos.x;
				float dz = spawnPos.z - playerPos.z;
				float distSq = dx * dx + dz * dz;

				if (distSq > (SAFE_DISTANCE * SAFE_DISTANCE)) {
					validPos = true; // 距离够远，位置有效
				}

				attempts++;
			}

			// 如果找到了有效位置，生成敌人
			if (validPos) {
				Enemy_Create(spawnPos);
			}
		}
	}
}

void Enemy_Finalize()
{
	for (auto* e : g_Enemies) delete e;
	g_Enemies.clear();
	EnemyTest::UnloadAssets();
}

void Enemy_Draw(DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj)
{
	SkinningShader_3D_Begin();
	for (auto* e : g_Enemies) {
		e->Draw(view,proj);
	}
}

void Enemy_Create(const XMFLOAT3& position)
{
	Enemy* newEnemy = new EnemyTest(position);
	g_Enemies.push_back(newEnemy);
}

int Enemy_GetEnemyCount()
{
	return (int)g_Enemies.size();
}

Enemy* Enemy_GetEnemy(int index)
{
	if (index < 0 || index >= g_Enemies.size()) return nullptr;
	return g_Enemies[index];
}
