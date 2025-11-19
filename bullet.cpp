#include "bullet.h"
#include "model.h"
using namespace DirectX;
#include "bullet_hit_effect.h"
class Bullet
{
private:
	XMFLOAT3 m_position{};
	XMFLOAT3 m_velocity{};
	double m_accumulatedTime{ 0.0 };
	static constexpr double LIFE_TIME = 3.0; // 弾の寿命（秒）


public:
	Bullet(const XMFLOAT3& pos, const XMFLOAT3& vel)
		:m_position(pos), m_velocity(vel)
	{
	}

	void Update(double elapsed_time)
	{
		XMStoreFloat3(&m_position, XMLoadFloat3(&m_position) + XMLoadFloat3(&m_velocity) * elapsed_time);
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
		return m_accumulatedTime >= LIFE_TIME;
	}


};

namespace
{
	constexpr int MAX_BULLET = 1024;
	Bullet* g_Bullets[MAX_BULLET]{ nullptr };
	int g_BulletCount = 0;
	MODEL* g_BulletModel{ nullptr };
}

void Bullet_Initialize()
{
	g_BulletModel = ModelLoad("resource/model/test.fbx", 0.05f);
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
		// 先更新位置
		g_Bullets[i]->Update(elapsed_time);

		// 检查是否死亡（寿命到了）
		if (g_Bullets[i]->isDestroy())
		{
			// [关键] 在销毁前生成特效
			BulletHitEffect_Create(g_Bullets[i]->GetPosition());

			// 释放内存
			delete g_Bullets[i];

			// Swap and Pop (用最后一个填补当前空缺)
			g_Bullets[i] = g_Bullets[g_BulletCount - 1];
			g_Bullets[g_BulletCount - 1] = nullptr;
			g_BulletCount--;
			i--;
		}
	}
}

void Bullet_Draw()
{

	XMMATRIX world;
	for (int i = 0; i < g_BulletCount; i++)
	{
		// 如果你想解决之前提到的子弹变形或朝向问题，请在这里应用 S*R*T 逻辑
		// 目前保持你原本的平移逻辑：
		world = XMMatrixTranslationFromVector(XMLoadFloat3(&g_Bullets[i]->GetPosition()));
		ModelDraw(g_BulletModel, world);
	}
}

void Bullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity)
{
	if (g_BulletCount >= MAX_BULLET) return;
	g_Bullets[g_BulletCount++] = new Bullet(position, velocity);
}

void Bullet_Destroy(int index)
{
	if (index < 0 || index >= g_BulletCount) return;
	BulletHitEffect_Create(g_Bullets[index]->GetPosition());
	delete g_Bullets[index];
	g_Bullets[index] = g_Bullets[g_BulletCount - 1];
	g_BulletCount--;
}

int Bullet_GetCount()
{
	return g_BulletCount;
}

AABB Bullet_GetAABB(int index)
{
	return ModelGetAABB(g_BulletModel, g_Bullets[index]->GetPosition());
}