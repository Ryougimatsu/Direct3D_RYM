#pragma once
#include <DirectXMath.h>
#include <vector>

// �������ӵ����ݽṹ
struct Particle {
	DirectX::XMFLOAT3 Position;  // λ��
	DirectX::XMFLOAT3 Velocity;  // �ٶ� (���� * ����)
	DirectX::XMFLOAT4 Color;     // ��ɫ (���� Alpha)
	float Age;                   // ��ǰ���ʱ��
	float LifeTime;              // ����������
	float Size;                  // ��С
	bool Active;                 // �Ƿ񼤻�
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