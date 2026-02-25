#include "particle_system.h"
#include "billboard.h"
#include <stdlib.h>

using namespace DirectX;

// ============================================================
// 工具函数：生成 [min, max] 范围内的随机浮点数
// ============================================================
float RandomFloat(float min, float max) {
	return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

// ============================================================
// 生命周期
// ============================================================

// 初始化粒子池：预分配指定数量的粒子，全部设为非激活状态
void ParticleSystem::Initialize(int maxParticles, int texID) {
	m_MaxParticles = maxParticles;
	m_TextureID = texID;
	m_Particles.resize(maxParticles);

	for (auto& p : m_Particles) {
		p.Active = false;
	}
}

// 清空粒子池，释放内存
void ParticleSystem::Finalize() {
	m_Particles.clear();
}

// ============================================================
// 每帧更新：遍历所有激活粒子，更新物理状态
// ============================================================
void ParticleSystem::Update(double dt) {
	for (auto& p : m_Particles) {
		if (!p.Active) continue;

		// 1. 累计存活时间，超过寿命则销毁
		p.Age += (float)dt;
		if (p.Age >= p.LifeTime) {
			p.Active = false;
			continue;
		}

		// 2. 应用重力 (仅 Y 轴速度受影响，血液粒子等需要此效果)
		p.Velocity.y -= p.Gravity * (float)dt;

		// 3. 根据速度更新位置 (欧拉积分: 位置 += 速度 * 时间步长)
		p.Position.x += p.Velocity.x * (float)dt;
		p.Position.y += p.Velocity.y * (float)dt;
		p.Position.z += p.Velocity.z * (float)dt;

		// 4. 尺寸随时间缓慢增长 (模拟扩散效果)
		p.Size += 0.5f * (float)dt;

		// 5. Alpha 透明度线性衰减 (从 1.0 渐变到 0.0，实现淡出效果)
		float lifeRatio = p.Age / p.LifeTime;
		p.Color.w = 1.0f - lifeRatio;
	}
}

// ============================================================
// 绘制：将所有激活粒子以公告板方式渲染
// ============================================================
void ParticleSystem::Draw() {
	for (const auto& p : m_Particles) {
		if (!p.Active) continue;

		// 使用带颜色参数的公告板绘制，粒子自动面向摄像机
		Billboard_Draw(
			m_TextureID,
			p.Position,
			{ p.Size, p.Size }, // 缩放 (宽, 高)
			{ 0.0f, 0.0f },     // 轴心点偏移
			p.Color             // RGBA 颜色 (包含 Alpha 淡出)
		);
	}
}

// ============================================================
// 发射器：通用爆炸粒子
// 橙色火焰，向四周随机扩散，适用于一般性爆炸特效
// ============================================================
void ParticleSystem::Emit(DirectX::XMFLOAT3 pos, int count) {
	int emittedCount = 0;

	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		p.Active = true;
		p.Position = pos;
		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.5f, 1.5f);
		p.Size = RandomFloat(0.5f, 1.0f);
		p.Color = { 1.0f, 0.5f, 0.2f, 1.0f }; // 橙色
		p.Gravity = 0.0f;

		// 初始速度：向四周随机方向爆散，Y 轴略偏向上方
		p.Velocity = {
			RandomFloat(-5.0f, 5.0f),
			RandomFloat(2.0f, 8.0f),
			RandomFloat(-5.0f, 5.0f)
		};

		emittedCount++;
	}
}

// ============================================================
// 发射器：烟雾粒子
// 灰色半透明，缓慢向上飘散，模拟战场硝烟 / 环境烟雾
// ============================================================
void ParticleSystem::EmitSmoke(DirectX::XMFLOAT3 pos, int count) {
	int emittedCount = 0;

	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		p.Active = true;
		p.Position = pos;

		// 在发射点周围随机偏移，制造分散感
		float offset = 0.5f;
		p.Position.x += RandomFloat(-offset, offset);
		p.Position.z += RandomFloat(-offset, offset);
		p.Position.y += RandomFloat(0.0f, 0.5f);

		p.Age = 0.0f;
		p.LifeTime = RandomFloat(3.0f, 5.0f);  // 烟雾持续较长时间
		p.Size = RandomFloat(2.0f, 3.0f);       // 初始尺寸较大
		p.Color = { 0.5f, 0.5f, 0.5f, 0.6f };  // 灰色半透明
		p.Gravity = 0.0f;                        // 不受重力影响

		// 速度：主要向上漂浮，水平方向轻微飘动
		p.Velocity = {
			RandomFloat(-0.5f, 0.5f),
			RandomFloat(1.0f, 2.5f),
			RandomFloat(-0.5f, 0.5f)
		};

		emittedCount++;
	}
}

// ============================================================
// 发射器：枪口闪光粒子
// 纯白色，零速度，极短寿命 (0.05~0.1秒)，模拟开火瞬间的强光
// ============================================================
void ParticleSystem::EmitMuzzleFlash(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count) {
	int emittedCount = 0;

	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		p.Active = true;

		// 沿枪管方向略微前推 0.2f，避免粒子嵌入枪管内部
		p.Position = {
			pos.x + dir.x * 0.2f,
			pos.y + dir.y * 0.2f,
			pos.z + dir.z * 0.2f
		};

		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.05f, 0.1f);  // 极短寿命，一闪即逝
		p.Size = RandomFloat(1.5f, 2.5f);
		p.Color = { 1.0f, 1.0f, 1.0f, 1.0f };  // 纯白色强光
		p.Gravity = 0.0f;
		p.Velocity = { 0.0f, 0.0f, 0.0f };      // 静止不动

		emittedCount++;
	}
}

// ============================================================
// 发射器：枪口火焰粒子
// 亮黄色，沿枪管方向喷射并带有侧向随机扩散，模拟开火时的火焰喷射
// ============================================================
void ParticleSystem::EmitMuzzleFire(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count) {
	int emittedCount = 0;

	// 构造枪管方向的局部正交坐标系 (用于计算侧向和上方扩散)
	XMVECTOR vDir = XMVector3Normalize(XMLoadFloat3(&dir));        // 前方 (枪管方向)
	XMVECTOR vUp = XMVectorSet(0, 1, 0, 0);                       // 参考上方向
	if (fabsf(XMVectorGetY(vDir)) > 0.99f) vUp = XMVectorSet(1, 0, 0, 0); // 防共线
	XMVECTOR vRight = XMVector3Normalize(XMVector3Cross(vUp, vDir)); // 右方向
	XMVECTOR vActualUp = XMVector3Cross(vDir, vRight);               // 实际上方向

	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		p.Active = true;

		// 沿枪管方向微小前推 0.05f，避免粒子嵌入枪管
		p.Position = {
			pos.x + dir.x * 0.05f,
			pos.y + dir.y * 0.05f,
			pos.z + dir.z * 0.05f
		};

		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.1f, 0.2f);           // 短寿命，快速闪烁
		p.Size = RandomFloat(1.0f, 2.0f);
		p.Color = { 1.0f, 0.85f, 0.2f, 1.0f };          // 亮黄色 (Alpha 会在 Update 中衰减)
		p.Gravity = 0.0f;                                // 火焰不受重力

		// 速度：沿枪管方向喷射 + 垂直/侧向随机扩散
		float forwardSpeed = RandomFloat(2.0f, 6.0f);    // 前向喷射速度
		float sideSpread = RandomFloat(-1.5f, 1.5f);     // 左右扩散
		float upSpread = RandomFloat(-1.5f, 1.5f);       // 上下扩散

		XMVECTOR vel = vDir * forwardSpeed + vRight * sideSpread + vActualUp * upSpread;
		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&p.Velocity), vel);

		emittedCount++;
	}
}

// ============================================================
// 发射器：血液粒子
// 暗红色，受重力影响呈抛物线下落，模拟敌人受击时的血液飞溅
// ============================================================
void ParticleSystem::EmitBlood(DirectX::XMFLOAT3 pos, int count) {
	int emittedCount = 0;
	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		p.Active = true;
		p.Position = { pos.x, pos.y + 1.0f, pos.z }; // 在敌人身体高度生成
		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.5f, 1.0f);
		p.Size = RandomFloat(0.8f, 1.5f);
		p.Color = { 0.45f, 0.0f, 0.0f, 0.9f };       // 暗红色

		p.Gravity = 9.8f;                              // 启用重力，模拟血液下落

		// 初始速度：向上喷涌 + 极小水平扩散
		p.Velocity = {
				RandomFloat(-0.3f, 0.3f),
				RandomFloat(1.5f, 3.5f),               // 向上的初速度
				RandomFloat(-0.3f, 0.3f)
		};
		emittedCount++;
	}
}
