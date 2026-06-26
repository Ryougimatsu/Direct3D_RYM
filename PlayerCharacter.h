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
#include "ExperienceComponent.h"
#include "PlayerExp.h"
#include "PlayerStats.h"
#include "Weapon.h"
#include "WeaponAttachmentComponent.h"
#include <vector>
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
	explicit PlayerCharacter(ExperienceConfig experienceConfig = {});
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
	bool IsDead() const { return m_Stats.currentHp <= 0.0f; }
	float GetHP() const { return m_Stats.currentHp; }

	// 死亡逻辑
	bool IsDeathAnimationFinished() const { return m_IsDeadFinished; }

	// ==========================================
	// 5. 武器与弹药 (Weapon & Ammo)
	// ==========================================
	void AddAmmo(int amount); // 捡到子弹时调用

	int GetCurrentAmmo() const { return m_CurrentAmmo; }
	int GetTotalAmmo() const { return m_TotalAmmo; }
	bool IsReloading() const { return m_IsReloading; }

	// ==========================================
	// 6. 成长系统 (Roguelike Growth)
	// ==========================================
	ExperienceGainResult AddExperience(
		std::uint64_t enemyBaseExperience,
		std::uint32_t enemyLevel);
	ExperienceComponent& GetExperienceComponent() { return m_Experience; }
	const ExperienceComponent& GetExperienceComponent() const { return m_Experience; }
	PlayerExp& GetPlayerExp() { return m_PlayerExp; }
	const PlayerExp& GetPlayerExp() const { return m_PlayerExp; }
	PlayerStats& GetStats() { return m_Stats; }
	const PlayerStats& GetStats() const { return m_Stats; }

private:
	DirectX::XMMATRIX GetCharacterWorldMatrix() const;
	void UpdateWeaponAttachment();
	DirectX::XMVECTOR GetWeaponAimDirection() const;
	void DrawWeaponAttachmentDebug(const DirectX::XMMATRIX& view);
	void SyncLegacyFieldsFromStats();

	// ==========================================
	// 资源与组件 (Components)
	// ==========================================
	SkinningModel* m_pModel = nullptr;
	Animator       m_Animator;
	std::vector<DirectX::XMMATRIX> m_FinalBoneMatrices;
	Weapon m_Weapon;
	WeaponAttachmentComponent m_WeaponAttachment;
	int m_MuzzleTexID = -1;           // 枪口闪光贴图 ID（备用，当前未激活使用）
	float m_MuzzleFlashTimer = 0.0f;  // 枪口闪光显示计时器（备用）
	ParticleSystem* m_pMuzzleFireSystem = nullptr; // 枪口火焰粒子系统（每次开枪时发射）
	int m_MuzzleFireTexID = -1;                    // 枪口火焰粒子所用纹理 ID
	ExperienceComponent m_Experience;              // 等级、经验和经验倍率
	PlayerExp m_PlayerExp;                         // Roguelike 升级队列用经验数据
	PlayerStats m_Stats;                           // 可被技能修改的集中式玩家属性

	// ==========================================
	// 配置参数 (Configuration / Settings)
	// ==========================================
	// 基础属性
	// Legacy mirrors kept during the PlayerStats migration.  New gameplay code
	// should read/write m_Stats instead; these can be removed after all old
	// call sites are migrated.
	float m_MaxHP = 100.0f;      // 最大生命值上限
	float m_Scale = 0.01f;       // 角色模型缩放系数（FBX 导出单位换算至游戏单位）
	float m_MoveSpeed = 1.15f;    // 角色移动速度（单位/秒）

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
	CharacterState m_CurrentState = CharacterState::Idle; // 当前角色状态（Idle/Running/Dead）
	float          m_StateTimer = 0.0f;      // 通用状态计时器（用于状态驻留时长控制）

	// 空间属性
	DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f }; // 世界空间中的位置
	float             m_RotationY = 0.0f;                  // 绕 Y 轴的旋转角度（弧度）

	// 生命值
	// Legacy mirror of m_Stats.currentHp; kept temporarily for low-risk migration.
	float m_HP = 100.0f; // 当前生命值（降至 0 时触发死亡状态）

	// 计时器与标志位
	float m_ShootTimer = 0.0f;      // 开火计时器
	float m_InvincibleTimer = 0.0f; // 当前剩余无敌时间
	float m_MeleeTimer = 0.0f;      // 近战攻击计时器
	float m_ReloadTimer = 0.0f;     // 换弹计时器
	float m_DeathTimer = 0.0f;      // 死亡逻辑计时器

	bool  m_IsDeadFinished = false; // 标记：死亡动画+计时器是否已全部结束（供场景判断是否切 GameOver）
	bool  m_IsReloading = false;   // 标记：当前是否正在换弹
	bool  m_DebugLaserHit = false;
	DirectX::XMFLOAT3 m_DebugLaserStart = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_DebugLaserEnd = { 0.0f, 0.0f, 0.0f };

	// 弹药数据
	int m_CurrentAmmo = 30;  // 当前弹匣内的子弹数
	int m_TotalAmmo = 160;   // 身上携带的备弹总数（不含弹匣内）
};

// ----------------------------------------------------------------
// 全局 C 风格接口函数（供其他模块访问玩家状态）
// ----------------------------------------------------------------
DirectX::XMFLOAT3 Player_GetPosition();                          // 获取玩家世界坐标
void Player_SetPosition(const DirectX::XMFLOAT3& pos);           // 强制设置玩家位置（传送等）
void Player_Damage(float damage);                                 // 对玩家造成伤害
void Player_Heal(float amount);                                   // 为玩家回血
void Player_AddAmmo(int count);                                   // 拾取弹药
PlayerCharacter* Player_GetInstance();                            // 获取单例指针（可为 nullptr）
bool Sound_GetLatest(DirectX::XMFLOAT3& outPos, float& outRadius); // 查询最新声音事件（供敌人 AI 侦听）
void Player_DrawDamageFlash();                                    // 在屏幕上绘制受伤红色闪光（由 HUD 调用）
