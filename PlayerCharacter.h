#pragma once

// ----------------------------------------------------------------
// Includes
// ----------------------------------------------------------------
#include <DirectXMath.h>
#include "SkinningModel.h"
#include "Animator.h"
#include "model.h"
#include "collision.h"
#include "particle_system.h"
// ----------------------------------------------------------------
// Enums
// ----------------------------------------------------------------
enum class CharacterState {
	Idle,
	Running,
	Dead
};

// ----------------------------------------------------------------
// Class Definition
// ----------------------------------------------------------------
class PlayerCharacter {
public:
	// ==========================================
	// 1. 生命周期 (Lifecycle)
	// ==========================================
	PlayerCharacter() = default;
	~PlayerCharacter();

	static void LoadAssets();
	static void UnloadAssets();

	bool Initialize();

	// ==========================================
	// 2. 核心循环 (Core Loop)
	// ==========================================
	void Update(double dt);
	void Draw(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);
	void DrawShadow(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj);

	// ==========================================
	// 3. 空间与物理 (Transform & Physics)
	// ==========================================
	void SetPosition(const DirectX::XMFLOAT3& pos) { m_Position = pos; }
	DirectX::XMFLOAT3 GetPosition() const { return m_Position; }

	// 修复了原本的排版问题
	DirectX::XMFLOAT3 GetRotation() const {
		return { 0.0f, m_RotationY, 0.0f };
	}

	AABB GetAABB() const;

	// [Public Variables] - 保持原始公开权限
	DirectX::XMFLOAT3 m_CurrentMoveDir = { 0, 0, 0 }; // 当前平滑后的移动向量
	float m_DampingSpeed = 10.0f;                     // 响应速度，值越大越灵敏

	// ==========================================
	// 4. 战斗与生命值 (Combat & Health)
	// ==========================================
	void ApplyDamage(float damage);
	void Heal(float amount);

	bool IsInvincible() const { return m_InvincibleTimer > 0.0f; }
	bool IsDead() const { return m_HP <= 0.0f; }
	float GetHP() const { return m_HP; }

	// 死亡逻辑
	bool IsDeathAnimationFinished() const { return m_IsDeadFinished; }

	// ==========================================
	// 5. 武器与弹药 (Weapon & Ammo)
	// ==========================================
	void AddAmmo(int amount); // 捡到子弹时调用

	int GetCurrentAmmo() const { return m_CurrentAmmo; }
	int GetTotalAmmo() const { return m_TotalAmmo; }
	bool IsReloading() const { return m_IsReloading; }

private:
	// ==========================================
	// 资源与组件 (Components)
	// ==========================================
	SkinningModel* m_pModel = nullptr;
	Animator       m_Animator;
	MODEL* m_pGunModel = nullptr; // 建议初始化为 nullptr
	int m_MuzzleTexID = -1;           // 枪口特效贴图
	float m_MuzzleFlashTimer = 0.0f;  // 枪口特效显示计时器

	// ==========================================
	// 配置参数 (Configuration / Settings)
	// ==========================================
	// 基础属性
	float m_MaxHP = 100.0f;
	float m_Scale = 0.01f;       // 修正未识别的关键：定义缩放系数
	float m_MoveSpeed = 2.0f;
	float m_GunScale = 1.0f;

	// 战斗参数
	const float m_InvincibleDuration = 1.0f; // 受到伤害后的无敌时长（秒）
	float m_FireRate = 0.1f;                 // 射击间隔（0.1秒代表1秒10发）

	// 弹药参数
	const int MAG_SIZE = 30;                 // 弹匣容量
	const float RELOAD_TIME = 2.0f;          // 换弹需要2秒

	// ==========================================
	// 运行时状态 (Runtime State)
	// ==========================================
	// 状态机
	CharacterState m_CurrentState = CharacterState::Idle;
	float          m_StateTimer = 0.0f;      // 用于控制测试流程的计时器

	// 空间属性
	DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
	float             m_RotationY = 0.0f;

	// 生命值
	float m_HP = 100.0f;

	// 计时器与标志位
	float m_ShootTimer = 0.0f;      // 开火计时器
	float m_InvincibleTimer = 0.0f; // 当前剩余无敌时间
	float m_MeleeTimer = 0.0f;      // 近战攻击计时器
	float m_ReloadTimer = 0.0f;     // 换弹计时器
	float m_DeathTimer = 0.0f;      // 死亡逻辑计时器

	bool  m_IsDeadFinished = false; // 标记：2秒倒计时是否结束
	bool  m_IsReloading = false;

	// 弹药数据
	int m_CurrentAmmo = 30;  // 当前弹匣内的子弹
	int m_TotalAmmo = 120;   // 备弹 (身上携带的总数，不含弹匣)
};

// ----------------------------------------------------------------
// Global Helper Functions
// ----------------------------------------------------------------
DirectX::XMFLOAT3 Player_GetPosition();
void Player_SetPosition(const DirectX::XMFLOAT3& pos);
void Player_Damage(float damage);
void Player_Heal(float amount);
void Player_AddAmmo(int count);
PlayerCharacter* Player_GetInstance();
bool Sound_GetLatest(DirectX::XMFLOAT3& outPos, float& outRadius);