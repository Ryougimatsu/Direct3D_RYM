#pragma once
#include <DirectXMath.h>
#include <vector>


// �������ӵ����ݽṹ
struct Particle {
	DirectX::XMFLOAT3 Position;  // 位置
	DirectX::XMFLOAT3 Velocity;  // 速度 
	DirectX::XMFLOAT4 Color;     // 颜色
	float Age;                   // 当前存活时间
	float LifeTime;              // 最大生存期
	float Size;                  // 大小
	bool Active;                 // 是否激活
	float Gravity;               // 该粒子受到的重力加速度
};

class ParticleSystem {
private:
	std::vector<Particle> m_Particles; // ���ӳ�
	int m_MaxParticles;
	int m_TextureID;                   // ����ʹ�õ����� ID

public:
	void Initialize(int maxParticles, int texID);
	void Finalize();
	void Update(double dt);
	void Draw();
	void Emit(DirectX::XMFLOAT3 pos, int count);
	void EmitSmoke(DirectX::XMFLOAT3 pos, int count);
	void EmitMuzzleFlash(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count);
	void EmitMuzzleFire(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count);
	void EmitBlood(DirectX::XMFLOAT3 pos, int count);
};