#include "particle_system.h"
#include "billboard.h"
#include <stdlib.h> 

using namespace DirectX;

// 辅助函数：生成随机浮点数 [min, max]
float RandomFloat(float min, float max) {
	return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

void ParticleSystem::Initialize(int maxParticles, int texID) {
	m_MaxParticles = maxParticles;
	m_TextureID = texID;
	m_Particles.resize(maxParticles);

	// 初始化所有粒子为非激活状态
	for (auto& p : m_Particles) {
		p.Active = false;
	}
}

void ParticleSystem::Finalize() {
	m_Particles.clear();
}

void ParticleSystem::Update(double dt) {
	for (auto& p : m_Particles) {
		if (!p.Active) continue;

		// 1. 更新生命周期
		p.Age += (float)dt;
		if (p.Age >= p.LifeTime) {
			p.Active = false; // 死亡
			continue;
		}

		// 2. 物理更新 (位置 += 速度 * 时间)
		p.Position.x += p.Velocity.x * (float)dt;
		p.Position.y += p.Velocity.y * (float)dt;
		p.Position.z += p.Velocity.z * (float)dt;

		// 3. 简单的重力效果 (可选)
		// p.Velocity.y -= 9.8f * (float)dt * 0.5f; 
		p.Size += 1.5f * (float)dt;

		// 4. 颜色淡出效果 (Alpha 随时间从 1 变到 0)
		float lifeRatio = p.Age / p.LifeTime;
		p.Color.w = 1.0f - lifeRatio; // Alpha
	}
}

void ParticleSystem::Draw() {
	// 开启混合模式 (通常在调用此函数前，在 Game_Draw 中开启 Additive 或 Alpha Blend)
	// 这里直接复用你的 Billboard_Draw

	for (const auto& p : m_Particles) {
		if (!p.Active) continue;

		// 调用刚才修改过的带颜色的 Billboard_Draw
		Billboard_Draw(
			m_TextureID,
			p.Position,
			{ p.Size, p.Size }, // Scale
			{ 0.0f, 0.0f },     // Pivot
			p.Color             // Color
		);
	}
}

void ParticleSystem::Emit(DirectX::XMFLOAT3 pos, int count) {
	int emittedCount = 0;

	// 寻找空闲的粒子进行发射
	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue; // 跳过正忙的粒子

		// 激活粒子
		p.Active = true;
		p.Position = pos;
		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.5f, 1.5f); // 随机存活 0.5~1.5秒
		p.Size = RandomFloat(0.5f, 1.0f);     // 随机大小
		p.Color = { 1.0f, 0.5f, 0.2f, 1.0f }; // 橙色 (火花)

		// 随机速度 (向四周炸开)
		p.Velocity = {
			RandomFloat(-5.0f, 5.0f),
			RandomFloat(2.0f, 8.0f),  // 稍微向上
			RandomFloat(-5.0f, 5.0f)
		};

		emittedCount++;
	}
}

void ParticleSystem::EmitSmoke(DirectX::XMFLOAT3 pos, int count) {
	int emittedCount = 0;

	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		// 1. 激活
		p.Active = true;
		p.Position = pos;

		// 2. 初始随机位置偏移 (让烟雾不要都挤在一个点生出来，而是有一点范围)
		float offset = 0.5f;
		p.Position.x += RandomFloat(-offset, offset);
		p.Position.z += RandomFloat(-offset, offset);
		p.Position.y += RandomFloat(0.0f, 0.5f); // 稍微离地一点

		p.Age = 0.0f;
		p.LifeTime = RandomFloat(3.0f, 5.0f); // 烟雾存活时间较长 (3-5秒)

		// 3. 初始大小 (从 2.0 到 3.0，比较大)
		p.Size = RandomFloat(2.0f, 3.0f);

		// 4. 颜色：灰灰色 (0.5, 0.5, 0.5)，半透明 (Alpha 0.6)
		// 战场烟雾通常比较暗，不是纯白的
		p.Color = { 0.5f, 0.5f, 0.5f, 0.6f };

		// 5. 速度：主要是缓慢向上，稍微带点随机风向
		p.Velocity = {
			RandomFloat(-0.5f, 0.5f), // X轴微风
			RandomFloat(1.0f, 2.5f),  // Y轴向上漂浮
			RandomFloat(-0.5f, 0.5f)  // Z轴微风
		};

		emittedCount++;
	}
}
