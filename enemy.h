#pragma once
#include <DirectXMath.h>
#include "collision.h"
#include <cstdint>
#include <vector>
#include "particle_system.h"

class DifficultyManager;

class Enemy {

protected:
	class State
	{

	public:
		virtual ~State() = default;
		virtual void Update(double elapsed_time) = 0;
		virtual void Draw() const = 0;
	};
	// navmesh 算出来的路径点
	std::vector<DirectX::XMFLOAT3> m_Path;
	int m_CurrentPathIndex = 0;
	float m_PathTimer = 0.0f;
	float m_MoveSpeed = 1.0f; 

private:
	State* m_pState = {};
	State* m_pNextState = {};


public:
	void MoveToTarget(const DirectX::XMFLOAT3& targetPos, double dt);

	virtual ~Enemy() = default;
	virtual void Update(double elapsed_time);
	virtual void Draw(DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj) const = 0;
	virtual void ChangeState(class State* pNextState) = 0;
	virtual void Damage(float damage, bool isMelee) = 0;
	virtual float GetHP() const = 0;
	virtual bool IsDead() const = 0;
	virtual bool IsAlerted() const = 0;
	virtual const DirectX::XMFLOAT3& GetPosition() const = 0;
	virtual void SetPosition(const DirectX::XMFLOAT3& pos) = 0;
	virtual void DrawShadow(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj) const = 0;
	virtual void ApplyKnockback(const DirectX::XMVECTOR& direction, float force) = 0;
	virtual DirectX::XMFLOAT3 GetRotation() const = 0;
	virtual AABB GetAABB() {
		DirectX::XMFLOAT3 pos = GetPosition(); return {
		{pos.x - 0.5f, pos.y, pos.z - 0.5f},
		{pos.x + 0.5f, pos.y + 1.0f, pos.z + 0.5f}
		};
	}
	virtual bool IsDestroyed() const = 0;
	virtual std::uint32_t GetLevel() const = 0;
	virtual std::uint64_t GetBaseExperience() const = 0;
	virtual void ApplyDifficultyScaling(float hpMultiplier, float damageMultiplier, float speedMultiplier) = 0;
	virtual Sphere GetCollisionSphere() const { return {}; }
};
void Enemy_ResolveCollisions();
void Enemy_Initialize();
void Enemy_Finalize();
void Enemy_Update(double elapsed_time, const DifficultyManager& difficultyManager);
void Enemy_Draw(DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj);
void Enemy_Create(
	const DirectX::XMFLOAT3& position,
	std::uint32_t level = 1,
	std::uint64_t baseExperience = 10,
	float hpMultiplier = 1.0f,
	float damageMultiplier = 1.0f,
	float speedMultiplier = 1.0f);
int Enemy_GetEnemyCount();
Enemy* Enemy_GetEnemy(int index);
void Enemy_ApplyMeleeDamage(const DirectX::XMFLOAT3& position, const DirectX::XMVECTOR& forward, float radius, float angle);
void Enemy_DrawShadow(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj);
void Enemy_EmitBlood(const DirectX::XMFLOAT3& pos, int count);
void Enemy_AwardDefeatExperience(const Enemy& enemy);
