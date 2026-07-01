#include "bullet.h"
#include "model.h"
using namespace DirectX;
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "collision.h"
#include "enemy.h"
#include "map.h" // 确保包含地图检测

class Bullet
{
private:
	XMFLOAT3 m_position{};
	XMFLOAT3 m_velocity{};
	float m_damage{ BULLET_DEFAULT_DAMAGE };
	int m_remainingPierce{ BULLET_DEFAULT_REMAINING_PIERCE };
	static constexpr int MAX_HIT_ENEMY_IDS = 16;
	std::array<std::uintptr_t, MAX_HIT_ENEMY_IDS> m_hitEnemyIds{};
	int m_hitEnemyCount = 0;
	double m_accumulatedTime{ 0.0 };
	static constexpr double LIFE_TIME = 3.0; // 子弹寿命（秒）
	bool m_deleteFlag = false;

public:
	Bullet() = default;

	void Reset(const XMFLOAT3& pos, const XMFLOAT3& vel, float damage, int remainingPierce)
	{
		m_position = pos;
		m_velocity = vel;
		m_damage = std::max(0.0f, damage);
		m_remainingPierce = std::max(0, remainingPierce);
		m_hitEnemyIds.fill(0);
		m_hitEnemyCount = 0;
		m_accumulatedTime = 0.0;
		m_deleteFlag = false;
	}

	void Update(double elapsed_time)
	{
		// 1. 计算下一帧的预期位置
		XMVECTOR vPos = XMLoadFloat3(&m_position);
		XMVECTOR vVel = XMLoadFloat3(&m_velocity);
		XMVECTOR vNextPos = vPos + vVel * (float)elapsed_time;

		XMFLOAT3 nextPos;
		XMStoreFloat3(&nextPos, vNextPos);

		// 2. 射线检测：防止穿墙
		if (Map_CheckLineOfSightBlocked(m_position, nextPos))
		{
			m_deleteFlag = true; // 标记销毁

			return;
		}

		m_position = nextPos;
		m_accumulatedTime += elapsed_time;
	}

	const XMFLOAT3& GetPosition() const { return m_position; }
	float GetDamage() const { return m_damage; }

	XMFLOAT3 GetFront() const
	{
		XMFLOAT3 front;
		XMVECTOR v = XMLoadFloat3(&m_velocity);
		if (XMVector3LengthSq(v).m128_f32[0] < 0.0001f) return { 0,0,1 };
		XMStoreFloat3(&front, XMVector3Normalize(XMLoadFloat3(&m_velocity)));
		return front;
	}

	bool isDestroy() const
	{
		return m_deleteFlag || (m_accumulatedTime >= LIFE_TIME);
	}

	bool HasHitEnemy(const Enemy* enemy) const
	{
		const std::uintptr_t enemyId = reinterpret_cast<std::uintptr_t>(enemy);
		return std::find(
			m_hitEnemyIds.begin(),
			m_hitEnemyIds.begin() + m_hitEnemyCount,
			enemyId) != m_hitEnemyIds.begin() + m_hitEnemyCount;
	}

	void RegisterEnemyHit(const Enemy* enemy)
	{
		if (m_hitEnemyCount >= MAX_HIT_ENEMY_IDS) return;

		const std::uintptr_t enemyId = reinterpret_cast<std::uintptr_t>(enemy);
		m_hitEnemyIds[m_hitEnemyCount++] = enemyId;
	}

	bool ConsumePierceOrShouldDestroy()
	{
		if (m_remainingPierce > 0)
		{
			m_remainingPierce--;
			return false;
		}

		m_deleteFlag = true;
		return true;
	}
};

namespace
{
	constexpr int MAX_BULLET = 1024;
	constexpr float COLLISION_GRID_CELL_SIZE = 4.0f;
	std::array<Bullet, MAX_BULLET> g_BulletPool{};
	std::array<bool, MAX_BULLET> g_BulletPoolUsed{};
	Bullet* g_Bullets[MAX_BULLET]{ nullptr };
	int g_BulletCount = 0;
	MODEL* g_BulletModel{ nullptr };
	constexpr float BULLET_SPEED = 60.0f;

	struct CellCoord
	{
		int x = 0;
		int z = 0;

		bool operator==(const CellCoord& other) const
		{
			return x == other.x && z == other.z;
		}
	};

	struct CellCoordHash
	{
		std::size_t operator()(const CellCoord& coord) const
		{
			const std::uint64_t x = static_cast<std::uint32_t>(coord.x);
			const std::uint64_t z = static_cast<std::uint32_t>(coord.z);
			return static_cast<std::size_t>((x << 32) ^ z);
		}
	};

	CellCoord ToCollisionCell(const XMFLOAT3& position)
	{
		return {
			static_cast<int>(std::floor(position.x / COLLISION_GRID_CELL_SIZE)),
			static_cast<int>(std::floor(position.z / COLLISION_GRID_CELL_SIZE))
		};
	}

	using EnemyCollisionGrid =
		std::unordered_map<CellCoord, std::vector<Enemy*>, CellCoordHash>;

	EnemyCollisionGrid BuildEnemyCollisionGrid()
	{
		EnemyCollisionGrid grid;
		grid.reserve(static_cast<std::size_t>(Enemy_GetEnemyCount()) * 2);

		for (int i = 0; i < Enemy_GetEnemyCount(); ++i)
		{
			Enemy* enemy = Enemy_GetEnemy(i);
			if (!enemy || enemy->IsDead()) continue;

			grid[ToCollisionCell(enemy->GetPosition())].push_back(enemy);
		}

		return grid;
	}
}

void Bullet_Initialize()
{
	g_BulletModel = ModelLoadS("resource/model/Bullet.fbx", 0.1f);
	g_BulletPoolUsed.fill(false);
	std::fill(std::begin(g_Bullets), std::end(g_Bullets), nullptr);
	g_BulletCount = 0;
}

void Bullet_Finalize()
{
	ModelRelease(g_BulletModel);
	g_BulletModel = nullptr;

	g_BulletPoolUsed.fill(false);
	std::fill(std::begin(g_Bullets), std::end(g_Bullets), nullptr);
	g_BulletCount = 0;
}

void Bullet_Update(double elapsed_time)
{
	for (int i = 0; i < g_BulletCount; i++)
	{
		g_Bullets[i]->Update(elapsed_time);

		// 检查子弹是否死亡（超时或撞墙）
		if (g_Bullets[i]->isDestroy())
		{
			Bullet_Destroy(i);
			i--; // 退格，保证不漏掉下一个
		}
	}
}

void Bullet_CheckCollisionWithEnemies()
{
	const EnemyCollisionGrid enemyGrid = BuildEnemyCollisionGrid();

	for (int i = 0; i < Bullet_GetCount(); i++)
	{
		Bullet* bullet = g_Bullets[i];
		Sphere bulletSphere = Bullet_GetSphere(i);
		const CellCoord bulletCell = ToCollisionCell(bulletSphere.center);
		bool bulletDestroyed = false;

		for (int cellZ = bulletCell.z - 1; cellZ <= bulletCell.z + 1 && !bulletDestroyed; ++cellZ)
		{
			for (int cellX = bulletCell.x - 1; cellX <= bulletCell.x + 1 && !bulletDestroyed; ++cellX)
			{
				const auto found = enemyGrid.find({ cellX, cellZ });
				if (found == enemyGrid.end()) continue;

				for (Enemy* pEnemy : found->second)
				{
					if (!pEnemy) continue;
					if (pEnemy->IsDead()) continue;
					if (bullet->HasHitEnemy(pEnemy)) continue;

					// 检测子弹球体与敌人 AABB 是否碰撞
					if (Collision_IsOverlapSphereAABB(bulletSphere, pEnemy->GetAABB()))
					{
						bool isAlive = !pEnemy->IsDead();
						bullet->RegisterEnemyHit(pEnemy);

						// 1. 敌人受伤/死亡逻辑
						pEnemy->Damage(bullet->GetDamage(), false);

						if (isAlive)
						{
							XMFLOAT3 hitPos = bullet->GetPosition();
							hitPos.y -= 1.5f;
							Enemy_EmitBlood(hitPos, 5);
						}

						// 2. 穿透次数用完才销毁；还有穿透时继续检测后续敌人
						if (bullet->ConsumePierceOrShouldDestroy())
						{
							Bullet_Destroy(i);
							i--;
							bulletDestroyed = true;
							break;
						}
					}
				}
			}
		}
	}
}

void Bullet_Draw()
{
	for (int i = 0; i < g_BulletCount; i++)
	{
		// 1. 获取位置
		XMVECTOR pos = XMLoadFloat3(&g_Bullets[i]->GetPosition());

		// 2. 获取前进方向
		XMFLOAT3 frontF3 = g_Bullets[i]->GetFront();
		XMVECTOR forward = XMLoadFloat3(&frontF3);

		// 3. 构建旋转矩阵
		XMVECTOR worldUp = XMVectorSet(0, 1, 0, 0);

		// 防止万向节锁
		if (fabs(XMVectorGetY(XMVector3Dot(forward, worldUp))) > 0.99f) {
			worldUp = XMVectorSet(1, 0, 0, 0);
		}

		XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, forward));
		XMVECTOR up = XMVector3Cross(forward, right);

		XMMATRIX rotation;
		rotation.r[0] = right;   // X轴
		rotation.r[1] = up;      // Y轴
		rotation.r[2] = forward; // Z轴
		rotation.r[3] = XMVectorSet(0, 0, 0, 1);

		// 4. 模型方向修正 (修正了空格)
		XMMATRIX correction = XMMatrixRotationX(XM_PIDIV2);

		// 5. 合成最终世界矩阵
		XMMATRIX world = correction * rotation * XMMatrixTranslationFromVector(pos);

		ModelDraw(g_BulletModel, world);
	}
}

void Bullet_Create(
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& velocity,
	float damage,
	int remainingPierce)
{
	if (g_BulletCount >= MAX_BULLET) return;

	XMVECTOR vDir = XMLoadFloat3(&velocity);
	if (XMVectorGetX(XMVector3LengthSq(vDir)) < 0.000001f) {
		return;
	}
	vDir = XMVector3Normalize(vDir);
	vDir = vDir * BULLET_SPEED; // 修正了空格

	XMFLOAT3 finalVelocity;
	XMStoreFloat3(&finalVelocity, vDir);

	for (int poolIndex = 0; poolIndex < MAX_BULLET; ++poolIndex)
	{
		if (g_BulletPoolUsed[poolIndex]) continue;

		g_BulletPoolUsed[poolIndex] = true;
		g_BulletPool[poolIndex].Reset(
			position,
			finalVelocity,
			damage,
			remainingPierce);
		g_Bullets[g_BulletCount++] = &g_BulletPool[poolIndex];
		return;
	}
}

void Bullet_Destroy(int index)
{
	if (index < 0 || index >= g_BulletCount) return;

	Bullet* bullet = g_Bullets[index];
	if (bullet >= g_BulletPool.data() &&
		bullet < g_BulletPool.data() + g_BulletPool.size())
	{
		const std::ptrdiff_t poolIndex = bullet - g_BulletPool.data();
		g_BulletPoolUsed[static_cast<std::size_t>(poolIndex)] = false;
	}

	g_Bullets[index] = g_Bullets[g_BulletCount - 1];
	g_Bullets[g_BulletCount - 1] = nullptr;
	g_BulletCount--;
}

int Bullet_GetCount()
{
	return g_BulletCount;
}

Sphere Bullet_GetSphere(int index)
{
	return { g_Bullets[index]->GetPosition(), 0.1f };
}

AABB Bullet_GetAABB(int index)
{
	return ModelGetAABB(g_BulletModel, g_Bullets[index]->GetPosition());
}
