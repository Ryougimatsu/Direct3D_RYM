#pragma once
#include "SkinningModel.h"
#include "Animator.h"
#include <DirectXMath.h>
#include "model.h"
#include "collision.h"


// 定义状态机状态
enum class CharacterState {
	Idle,
	Running,
	Dead
};

class PlayerCharacter {
public:
	PlayerCharacter() = default;
	~PlayerCharacter();

	bool Initialize();
	void Update(double dt);
	void Draw(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);
	DirectX::XMFLOAT3 GetPosition() const { return m_Position; }
	DirectX::XMFLOAT3 GetRotation() const {return { 0.0f, m_RotationY, 0.0f };
	}
	void SetPosition(const DirectX::XMFLOAT3& pos) { m_Position = pos; }

	DirectX::XMFLOAT3 m_CurrentMoveDir = { 0, 0, 0 }; // 当前平滑后的移动向量
	float m_DampingSpeed = 10.0f; // 响应速度，值越大越灵敏

	bool IsInvincible() const { return m_InvincibleTimer > 0.0f; }
	void ApplyDamage(float damage);
	float GetHP() const { return m_HP; }
	void Heal(float amount);
	bool IsDead() const { return m_HP <= 0.0f; }

	int GetCurrentAmmo() const { return m_CurrentAmmo; }
	int GetTotalAmmo() const { return m_TotalAmmo; } 
	bool IsReloading() const { return m_IsReloading; }
	void AddAmmo(int amount); // 捡到子弹时调用

	AABB GetAABB() const;

	bool IsDeathAnimationFinished() const { return m_IsDeadFinished; }

private:
	// 资源与组件
	SkinningModel* m_pModel = nullptr;
	Animator       m_Animator;

	MODEL* m_pGunModel;
	// 状态机变量
	CharacterState m_CurrentState = CharacterState::Idle;
	float          m_StateTimer = 0.0f; // 用于控制测试流程的计时器

	// 空间属性
	DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
	float m_HP = 100.0f;
	float m_MaxHP = 100.0f;
	float m_RotationY = 0.0f;
	float m_Scale = 0.01f;     // 修正未识别的关键：定义缩放系数
	float m_MoveSpeed = 2.0f;
	float m_GunScale = 1.0f;
	float m_ShootTimer = 0.0f;          // 开火计时器
	float m_FireRate = 0.1f;           // 射击间隔（0.1秒代表1秒10发）
	float m_InvincibleTimer = 0.0f;      // 当前剩余无敌时间
	float m_MeleeTimer = 0.0f;           // 近战攻击计时器
	const float m_InvincibleDuration = 1.0f; // 受到伤害后的无敌时长（秒）
	bool  m_IsDeadFinished = false; // 标记：2秒倒计时是否结束
	float m_DeathTimer = 0.0f;      // 计时器

	
	const int MAG_SIZE = 30;     // 弹匣容量
	int m_CurrentAmmo = 30;      // 当前弹匣内的子弹
	int m_TotalAmmo = 120;       // 备弹 (身上携带的总数，不含弹匣)

	bool m_IsReloading = false;
	float m_ReloadTimer = 0.0f;
	const float RELOAD_TIME = 2.0f; // 换弹需要2秒
};

DirectX::XMFLOAT3 Player_GetPosition();
void Player_Damage(float damage);
PlayerCharacter* Player_GetInstance();
bool Sound_GetLatest(DirectX::XMFLOAT3& outPos, float& outRadius);
void Player_SetPosition(const DirectX::XMFLOAT3& pos);
void Player_AddAmmo(int count);
void Player_Heal(float amount);