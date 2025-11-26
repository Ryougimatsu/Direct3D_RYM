#pragma once
#include "enemy.h"
#include <DirectXMath.h>
#include "texture.h"


class EnemyTest : public Enemy {
private:
	DirectX::XMFLOAT3 m_position{};
	float m_DetectionAngle = 5.0f;
	float m_HP = 100.0f;
	int m_TexID{};
public:
	EnemyTest(const DirectX::XMFLOAT3& position) : m_position(position) {
		m_TexID = Texture_LoadFromFile(L"resource/texture/Cube_Draw.png");
		ChangeState(new EnemyTest_StatePatrol(this));
	}

	bool IsDestroyed() const override
	{
		return m_HP <= 0.0f;
	}
private:
	class EnemyTest_StatePatrol : public State {
	private:
		EnemyTest* m_pOwner = {};
		float m_PointX = {};
		double m_AccumulatedTime = {};
	public:
		EnemyTest_StatePatrol(EnemyTest* pOwner)
			: m_pOwner(pOwner)
			, m_PointX(pOwner->m_position.x){}
		void Update(double elapsed_time) override;
		void Draw() const override;
	};

	class EnemyTest_StateChase : public State {
	private:
		EnemyTest* m_pOwner = {};
		double m_AccumulatedTime = {};
	public:
		EnemyTest_StateChase(EnemyTest* pOwner)
			: m_pOwner(pOwner){
		}
		void Update(double elapsed_time) override;
		void Draw() const override;
	};
};