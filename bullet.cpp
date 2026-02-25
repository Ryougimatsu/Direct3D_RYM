#include "bullet.h"
#include "model.h"
using namespace DirectX;
#include "bullet_hit_effect.h"
#include "collision.h"
#include "enemy.h"
#include "map.h" // 确保包含地图检测

class Bullet
{
private:
	XMFLOAT3 m_position{};
	XMFLOAT3 m_velocity{};
	double m_accumulatedTime{ 0.0 };
	static constexpr double LIFE_TIME = 3.0; // 子弹寿命（秒）
	bool m_deleteFlag = false;

public:
	Bullet(const XMFLOAT3& pos, const XMFLOAT3& vel)
		:m_position(pos), m_velocity(vel)
	{
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

			// 【修改】这里不要调用 BulletHitEffect_Create，
			// 因为 Bullet_Destroy 中已经统一调用了。
			// BulletHitEffect_Create(m_position); 
			return;
		}

		m_position = nextPos;
		m_accumulatedTime += elapsed_time;
	}

	const XMFLOAT3& GetPosition() const { return m_position; }

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
};

namespace
{
	constexpr int MAX_BULLET = 1024;
	Bullet* g_Bullets[MAX_BULLET]{ nullptr };
	int g_BulletCount = 0;
	MODEL* g_BulletModel{ nullptr };
	constexpr float BULLET_SPEED = 60.0f;
}

void Bullet_Initialize()
{
	g_BulletModel = ModelLoadS("resource/model/Bullet.fbx", 0.1f);
	for (int i = 0; i < g_BulletCount; i++)
	{
		delete g_Bullets[i];
		g_Bullets[i] = nullptr;
	}
	g_BulletCount = 0;
}

void Bullet_Finalize()
{
	ModelRelease(g_BulletModel);

	for (int i = 0; i < g_BulletCount; i++)
	{
		delete g_Bullets[i];
		g_Bullets[i] = nullptr;
	}
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
	for (int i = 0; i < Bullet_GetCount(); i++)
	{
		Sphere bulletSphere = Bullet_GetSphere(i);

		for (int j = 0; j < Enemy_GetEnemyCount(); j++)
		{
			Enemy* pEnemy = Enemy_GetEnemy(j);
			if (!pEnemy) continue;

			// 检测子弹球体与敌人 AABB 是否碰撞
			if (Collision_IsOverlapSphereAABB(bulletSphere, pEnemy->GetAABB()))
			{

				bool isAlive = !pEnemy->IsDead();
				// 1. 敌人受伤/死亡逻辑
				pEnemy->Damage(10.0f, false); // 假设每次命中造成 10 点伤害

				if (isAlive)
				{
					XMFLOAT3 hitPos = g_Bullets[i]->GetPosition();
					hitPos.y -= 1.5f;
					Enemy_EmitBlood(hitPos, 5);
				}

				// 2. 销毁子弹
				Bullet_Destroy(i);

				// 3. 回退索引并跳出
				i--;
				break;
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

void Bullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity)
{
	if (g_BulletCount >= MAX_BULLET) return;

	XMVECTOR vDir = XMLoadFloat3(&velocity);
	vDir = XMVector3Normalize(vDir);
	vDir = vDir * BULLET_SPEED; // 修正了空格

	XMFLOAT3 finalVelocity;
	XMStoreFloat3(&finalVelocity, vDir);

	g_Bullets[g_BulletCount++] = new Bullet(position, finalVelocity);
}

void Bullet_Destroy(int index)
{
	if (index < 0 || index >= g_BulletCount) return;

	// 这里统一生成击中特效 (无论是撞墙、撞人还是超时)
	// 如果希望超时不生成特效，可以在这里加判断
	BulletHitEffect_Create(g_Bullets[index]->GetPosition());

	delete g_Bullets[index];
	g_Bullets[index] = g_Bullets[g_BulletCount - 1];
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