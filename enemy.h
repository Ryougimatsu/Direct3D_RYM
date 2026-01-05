#pragma once
#include <DirectXMath.h>
#include "collision.h"
class Enemy {

protected:
	class State
	{

	public:
		virtual ~State() = default;
		virtual void Update(double elapsed_time) = 0;
		virtual void Draw() const = 0;
	};

private:
	State* m_pState = {};
	State* m_pNextState = {};
public:
	virtual ~Enemy() = default;
	virtual void Update(double elapsed_time);
	virtual void Draw(DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj) const = 0;
	virtual void ChangeState(class State* pNextState) = 0;
	//virtual void UpdateState();
	virtual void Damage(float) {}
	virtual bool IsAlerted() const = 0;
	virtual const DirectX::XMFLOAT3& GetPosition() const = 0;
	virtual void SetPosition(const DirectX::XMFLOAT3& pos) = 0;
	virtual void ApplyKnockback(const DirectX::XMVECTOR& direction, float force) = 0;
	virtual DirectX::XMFLOAT3 GetRotation() const = 0;
	virtual AABB GetAABB() {
		DirectX::XMFLOAT3 pos = GetPosition(); return {
		{pos.x - 0.5f, pos.y, pos.z - 0.5f},
		{pos.x + 0.5f, pos.y + 1.0f, pos.z + 0.5f}
		};
	}
	virtual bool IsDestroyed() const = 0;
	virtual Sphere GetCollisionSphere() const { return {}; }
};

void Enemy_Initialize();
void Enemy_Finalize();
void Enemy_Update(double elapsed_time);
void Enemy_Draw(DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj);
void Enemy_Create(const DirectX::XMFLOAT3& position);
int Enemy_GetEnemyCount();
Enemy* Enemy_GetEnemy(int index);
void Enemy_ApplyMeleeDamage(const DirectX::XMFLOAT3& position, const DirectX::XMVECTOR& forward, float radius, float angle);