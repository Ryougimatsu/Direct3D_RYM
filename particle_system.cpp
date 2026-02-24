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

		// 1. ������������
		p.Age += (float)dt;
		if (p.Age >= p.LifeTime) {
			p.Active = false; // ����
			continue;
		}

		// 2. ������� (λ�� += �ٶ� * ʱ��)
		p.Position.x += p.Velocity.x * (float)dt;
		p.Position.y += p.Velocity.y * (float)dt;
		p.Position.z += p.Velocity.z * (float)dt;

		// 3. �򵥵�����Ч�� (��ѡ)
		//p.Velocity.y -= 9.8f * (float)dt * 0.5f; 
		p.Size += 1.5f * (float)dt;

		// 4. ��ɫ����Ч�� (Alpha ��ʱ��� 1 �䵽 0)
		float lifeRatio = p.Age / p.LifeTime;
		p.Color.w = 1.0f - lifeRatio; // Alpha
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

	// Ѱ�ҿ��е����ӽ��з���
	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		// ��������
		p.Active = true;
		p.Position = pos;
		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.5f, 1.5f); // ������ 0.5~1.5��
		p.Size = RandomFloat(0.5f, 1.0f);     // �����С
		p.Color = { 1.0f, 0.5f, 0.2f, 1.0f }; // ��ɫ (��)

		// ����ٶ� (������ը��)
		p.Velocity = {
			RandomFloat(-5.0f, 5.0f),
			RandomFloat(2.0f, 8.0f),  // ��΢����
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

		// 1. ����
		p.Active = true;
		p.Position = pos;

		// 2. ��ʼ���λ��ƫ�� (�������Ҫ������һ������������������һ�㷶Χ)
		float offset = 0.5f;
		p.Position.x += RandomFloat(-offset, offset);
		p.Position.z += RandomFloat(-offset, offset);
		p.Position.y += RandomFloat(0.0f, 0.5f); // ��΢���һ��

		p.Age = 0.0f;
		p.LifeTime = RandomFloat(3.0f, 5.0f); // ������ʱ��ϳ� (3-5��)

		// 3. ��ʼ��С (�� 2.0 �� 3.0���Ƚϴ�)
		p.Size = RandomFloat(2.0f, 3.0f);

		// 4. ��ɫ���һ�ɫ (0.5, 0.5, 0.5)����͸�� (Alpha 0.6)
		// ս������ͨ���Ƚϰ������Ǵ��׵�
		p.Color = { 0.5f, 0.5f, 0.5f, 0.6f };

		// 5. �ٶȣ���Ҫ�ǻ������ϣ���΢�����������
		p.Velocity = {
			RandomFloat(-0.5f, 0.5f), // X��΢��
			RandomFloat(1.0f, 2.5f),  // Y������Ư��
			RandomFloat(-0.5f, 0.5f)  // Z��΢��
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
		// 稍微将粒子沿着子弹方向往前推一点，避免特效嵌在枪管里
		p.Position = {
			pos.x + dir.x * 0.2f,
			pos.y + dir.y * 0.2f,
			pos.z + dir.z * 0.2f
		};

		p.Age = 0.0f;
		// 寿命极短，通常 0.05 秒到 0.1 秒即可
		p.LifeTime = RandomFloat(0.05f, 0.1f);

		// 根据你的图片大小调整，可以随机大小增加爆发感
		p.Size = RandomFloat(1.5f, 2.5f);

		// 颜色保持原图颜色 (设为纯白，Alpha 1.0)
		p.Color = { 1.0f, 1.0f, 1.0f, 1.0f };

		// 枪口火焰不需要大幅度移动，给一个极小的初速度甚至 0 都可以
		p.Velocity = { 0.0f, 0.0f, 0.0f };

		emittedCount++;
	}
}

void ParticleSystem::EmitMuzzleFire(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count) {
	int emittedCount = 0;

	// 构造枪管方向的正交基 (用于侧向扩散)
	XMVECTOR vDir = XMVector3Normalize(XMLoadFloat3(&dir));
	XMVECTOR vUp = XMVectorSet(0, 1, 0, 0);
	// 若枪管几乎朝正上方, 换一个参考轴
	if (fabsf(XMVectorGetY(vDir)) > 0.99f)
		vUp = XMVectorSet(1, 0, 0, 0);
	XMVECTOR vRight = XMVector3Normalize(XMVector3Cross(vUp, vDir));
	XMVECTOR vActualUp = XMVector3Cross(vDir, vRight);

	for (auto& p : m_Particles) {
		if (emittedCount >= count) break;
		if (p.Active) continue;

		p.Active = true;
		// 沿枪管方向偏移 0.1f 避免嵌入枪管
		p.Position = {
			pos.x + dir.x * 0.1f,
			pos.y + dir.y * 0.1f,
			pos.z + dir.z * 0.1f
		};

		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.08f, 0.2f);
		p.Size = RandomFloat(0.6f, 1.2f);

		// 亮黄色, alpha 在 Update 中会自动衰减产生橙色→透明过渡
		p.Color = { 1.0f, 0.85f, 0.2f, 1.0f };

		// 沿枪管方向喷射 + 侧向随机扩散
		float forwardSpeed = RandomFloat(3.0f, 10.0f);
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
		p.Position = pos;
		p.Age = 0.0f;
		p.LifeTime = RandomFloat(0.4f, 0.8f);
		p.Size = RandomFloat(0.2f, 0.5f);
		p.Color = { 0.6f, 0.0f, 0.0f, 1.0f }; // 暗红色代表血液

		// 向上四周喷溅
		p.Velocity = {
			RandomFloat(-3.0f, 3.0f),
			RandomFloat(2.0f, 6.0f),
			RandomFloat(-3.0f, 3.0f)
		};
		emittedCount++;
	}
}
