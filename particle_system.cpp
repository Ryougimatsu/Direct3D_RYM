#include "particle_system.h"
#include "billboard.h"
#include <stdlib.h> 

using namespace DirectX;
float RandomFloat(float min, float max) {
	return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

void ParticleSystem::Initialize(int maxParticles, int texID) {
	m_MaxParticles = maxParticles;
	m_TextureID = texID;
	m_Particles.resize(maxParticles);

	// ��ʼ����������Ϊ�Ǽ���״̬
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

		p.Age += (float)dt;
		if (p.Age >= p.LifeTime) {
			p.Active = false;
			continue;
		}

		// 【新增】应用重力（每一帧让Y轴速度减小）
		p.Velocity.y -= p.Gravity * (float)dt;

		// 计算位置 (位置 += 速度 * 时间)
		p.Position.x += p.Velocity.x * (float)dt;
		p.Position.y += p.Velocity.y * (float)dt;
		p.Position.z += p.Velocity.z * (float)dt;

		// 尺寸稍微变大一点点即可，不需要像烟雾扩散那么快
		p.Size += 0.5f * (float)dt;

		// 颜色渐变效果 (Alpha 随时间从 1 变到 0)
		float lifeRatio = p.Age / p.LifeTime;
		p.Color.w = 1.0f - lifeRatio;
	}
}

void ParticleSystem::Draw() {
	for (const auto& p : m_Particles) {
		if (!p.Active) continue;

		// ���øղ��޸Ĺ��Ĵ���ɫ�� Billboard_Draw
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
		if (p.Active) continue;

		// 基础属性
		p.Active = true;
		p.Position = pos;
		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.5f, 1.5f);
		p.Size = RandomFloat(0.5f, 1.0f);
		p.Color = { 1.0f, 0.5f, 0.2f, 1.0f };

		// 【新增】重置重力，防止被血液粒子污染
		p.Gravity = 0.0f;

		// 初始速度 (向四周爆炸)
		p.Velocity = {
			RandomFloat(-5.0f, 5.0f),
			RandomFloat(2.0f, 8.0f),
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

		// 2. 初始化位置偏移
		float offset = 0.5f;
		p.Position.x += RandomFloat(-offset, offset);
		p.Position.z += RandomFloat(-offset, offset);
		p.Position.y += RandomFloat(0.0f, 0.5f);

		p.Age = 0.0f;
		p.LifeTime = RandomFloat(3.0f, 5.0f);
		p.Size = RandomFloat(2.0f, 3.0f);
		p.Color = { 0.5f, 0.5f, 0.5f, 0.6f };

		// 【新增】重置重力，烟雾不受重力影响而是向上飘
		p.Gravity = 0.0f;

		// 5. 速度：主要是向上飘
		p.Velocity = {
			RandomFloat(-0.5f, 0.5f),
			RandomFloat(1.0f, 2.5f),
			RandomFloat(-0.5f, 0.5f)
		};

		emittedCount++;
	}
}

void ParticleSystem::EmitMuzzleFlash(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count) {
	int emittedCount = 0;

	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		p.Active = true;
		p.Position = {
			pos.x + dir.x * 0.2f,
			pos.y + dir.y * 0.2f,
			pos.z + dir.z * 0.2f
		};

		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.05f, 0.1f);
		p.Size = RandomFloat(1.5f, 2.5f);
		p.Color = { 1.0f, 1.0f, 1.0f, 1.0f };

		// 【新增】重置重力
		p.Gravity = 0.0f;

		p.Velocity = { 0.0f, 0.0f, 0.0f };

		emittedCount++;
	}
}

void ParticleSystem::EmitMuzzleFire(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count) {
	int emittedCount = 0;

	XMVECTOR vDir = XMVector3Normalize(XMLoadFloat3(&dir));
	XMVECTOR vUp = XMVectorSet(0, 1, 0, 0);
	if (fabsf(XMVectorGetY(vDir)) > 0.99f) vUp = XMVectorSet(1, 0, 0, 0);
	XMVECTOR vRight = XMVector3Normalize(XMVector3Cross(vUp, vDir));
	XMVECTOR vActualUp = XMVector3Cross(vDir, vRight);

	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		p.Active = true;
		p.Position = {
			pos.x + dir.x * 0.05f,
			pos.y + dir.y * 0.05f,
			pos.z + dir.z * 0.05f
		};

		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.1f, 0.2f);
		p.Size = RandomFloat(1.0f, 2.0f);
		p.Color = { 1.0f, 0.85f, 0.2f, 1.0f };

		// 【新增】重置重力，让火花保持直线喷射
		p.Gravity = 0.0f;

		float forwardSpeed = RandomFloat(2.0f, 6.0f);
		float sideSpread = RandomFloat(-1.5f, 1.5f);
		float upSpread = RandomFloat(-1.5f, 1.5f);

		XMVECTOR vel = vDir * forwardSpeed + vRight * sideSpread + vActualUp * upSpread;
		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&p.Velocity), vel);

		emittedCount++;
	}
}

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

		p.Color = { 0.45f, 0.0f, 0.0f, 0.9f };

		p.Gravity = 9.8f;

		p.Velocity = {
				RandomFloat(-0.3f, 0.3f), // 几乎没有水平扩散
				RandomFloat(1.5f, 3.5f),  // 往上喷涌的初速度
				RandomFloat(-0.3f, 0.3f)
		};
		emittedCount++;
	}
}
