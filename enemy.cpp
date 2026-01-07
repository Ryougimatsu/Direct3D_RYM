#include "enemy.h"
#include "enemy_test.h"
#include "PlayerCharacter.h"
#include <vector>
#include <cstdlib>     
#include <ctime>
#include "SkinningShader.h"
#include "Meshfield.h"
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

void Enemy_ResolveCollisions() {
	const float ENEMY_RADIUS = 0.4f;
	const float PLAYER_RADIUS = 0.5f;
	const float MIN_DIST_E2E = ENEMY_RADIUS * 2.0f;
	const float MIN_DIST_E2P = ENEMY_RADIUS + PLAYER_RADIUS;

	// 获取玩家当前位置
	XMFLOAT3 pPos = Player_GetPosition();
	XMVECTOR vPlayerPos = XMLoadFloat3(&pPos);

	for (size_t i = 0; i < g_Enemies.size(); ++i) {
		// --- 1. 敌人与敌人之间的排斥 (保持不变，防止敌人重叠) ---
		for (size_t j = i + 1; j < g_Enemies.size(); ++j) {
			XMVECTOR posA = XMLoadFloat3(&g_Enemies[i]->GetPosition());
			XMVECTOR posB = XMLoadFloat3(&g_Enemies[j]->GetPosition());

			XMVECTOR diff = XMVectorSubtract(posA, posB);
			diff = XMVectorSetY(diff, 0.0f);

			float distSq = XMVectorGetX(XMVector3LengthSq(diff));
			if (distSq < MIN_DIST_E2E * MIN_DIST_E2E && distSq > 0.00001f) {
				float dist = sqrtf(distSq);
				float overlap = MIN_DIST_E2E - dist;
				XMVECTOR pushDir = XMVector3Normalize(diff);

				// 互相推开
				XMFLOAT3 newA, newB;
				XMStoreFloat3(&newA, posA + pushDir * overlap * 0.5f);
				XMStoreFloat3(&newB, posB - pushDir * overlap * 0.5f);

				g_Enemies[i]->SetPosition(newA);
				g_Enemies[j]->SetPosition(newB);
			}
		}

		// --- 2. 敌人与玩家之间的排斥 (修改此处) ---
		XMVECTOR posE = XMLoadFloat3(&g_Enemies[i]->GetPosition());
		XMVECTOR diffToP = XMVectorSubtract(posE, vPlayerPos); // 向量：玩家 -> 敌人
		diffToP = XMVectorSetY(diffToP, 0.0f);

		float distSqToP = XMVectorGetX(XMVector3LengthSq(diffToP));

		// 判定重叠
		if (distSqToP < MIN_DIST_E2P * MIN_DIST_E2P && distSqToP > 0.00001f) {
			float dist = sqrtf(distSqToP);
			float overlap = MIN_DIST_E2P - dist;

			// pushDir 是 "从玩家指向敌人" 的方向
			XMVECTOR pushDir = XMVector3Normalize(diffToP);

	
			// 计算玩家被推开后的新位置
			vPlayerPos = vPlayerPos - pushDir * overlap;

			// 应用位置到玩家
			XMFLOAT3 newP;
			XMStoreFloat3(&newP, vPlayerPos);
			Player_SetPosition(newP);

		}
	}
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

		if ((*it)->IsDestroyed())
		{
			delete (*it);
			it = g_Enemies.erase(it); 
		}
		else
		{
			++it;
		}
	}

	Enemy_ResolveCollisions();

	// ==========================================
	// 随机生成逻辑
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
	//EnemyTest::UnloadAssets();
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

void Enemy_ApplyMeleeDamage(const XMFLOAT3& pPos, const XMVECTOR& playerFwd, float range, float angle) {
	XMVECTOR vOrigin = XMLoadFloat3(&pPos);
	float cosThreshold = cosf(XMConvertToRadians(angle * 0.5f));

	for (auto* enemy : g_Enemies) {
		XMVECTOR vEnemyPos = XMLoadFloat3(&enemy->GetPosition());
		XMVECTOR diff = vEnemyPos - vOrigin;
		diff = XMVectorSetY(diff, 0.0f); // 忽略高度

		// [优化 2] 使用 LengthSq 避免开方运算，提高性能
		float distSq = XMVectorGetX(XMVector3LengthSq(diff));
		if (distSq < range * range) { // 比较距离的平方

			XMVECTOR dirToEnemy = XMVector3Normalize(diff);
			float dotFacing = XMVectorGetX(XMVector3Dot(playerFwd, dirToEnemy));

			// 判定：玩家是否面向敌人 (在扇形攻击范围内)
			if (dotFacing > cosThreshold) {

				// === 背刺逻辑优化 ===

				float enemyRotY = enemy->GetRotation().y;
				XMVECTOR enemyFwd = XMVectorSet(sinf(enemyRotY), 0, cosf(enemyRotY), 0);

				// 计算玩家朝向和敌人朝向的点积
				// 如果 > 0.5，说明两人朝向大致相同 -> 玩家在敌人背后
				float backstabDot = XMVectorGetX(XMVector3Dot(playerFwd, enemyFwd));

				bool isBackstab = (backstabDot > 0.5f);
				bool isAlerted = enemy->IsAlerted();

				float finalDamage = 0.0f;

				if (isBackstab && !isAlerted) {
					// --- 潜行击杀 ---
					finalDamage = 100.0f; // 致命一击
					// PlaySound("Stab.wav"); 
				}
				else {
					// --- 正面/警觉攻击 ---
					finalDamage = 40.0f; // 普通伤害

					// 施加击退
					enemy->ApplyKnockback(dirToEnemy, 2.0f);
				}

				// 应用伤害
				enemy->Damage(finalDamage, true);
			}
		}
	}
}
