#pragma once
#include <DirectXMath.h>
#include <vector>

// 单个粒子的数据结构
struct Particle {
	DirectX::XMFLOAT3 Position;  // 位置
	DirectX::XMFLOAT3 Velocity;  // 速度 (方向 * 速率)
	DirectX::XMFLOAT4 Color;     // 颜色 (包含 Alpha)
	float Age;                   // 当前存活时间
	float LifeTime;              // 总生命周期
	float Size;                  // 大小
	bool Active;                 // 是否激活
};

class ParticleSystem {
private:
	std::vector<Particle> m_Particles; // 粒子池
	int m_MaxParticles;
	int m_TextureID;                   // 粒子使用的纹理 ID

public:
	void Initialize(int maxParticles, int texID);
	void Finalize();
	void Update(double dt);
	void Draw();
	void Emit(DirectX::XMFLOAT3 pos, int count);
	void EmitSmoke(DirectX::XMFLOAT3 pos, int count);
};