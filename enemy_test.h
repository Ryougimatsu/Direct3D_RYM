#pragma once

// ----------------------------------------------------------------
// Includes
// ----------------------------------------------------------------
#include <vector>
#include <cstdint>
#include <DirectXMath.h>
#include "enemy.h"
#include "model.h"
#include "Animator.h"
#include "SkinningModel.h"
#include "Player_Camera.h"

// ----------------------------------------------------------------
// External Dependencies
// ----------------------------------------------------------------
extern bool Game_IsLineOfSightBlocked(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end);

// ----------------------------------------------------------------
// Class Definition
// ----------------------------------------------------------------
class EnemyTest : public Enemy {
public:
	// ==========================================
	// 1. 生命周期 (Lifecycle)
	// ==========================================
	EnemyTest(
		const DirectX::XMFLOAT3& position,
		std::uint32_t level = 1,
		std::uint64_t baseExperience = 25);
	~EnemyTest() override;

	static void LoadAssets();
	static void UnloadAssets();

	// ==========================================
	// 2. 核心循环 (Core Loop)
	// ==========================================
	void Update(double elapsed_time) override;
	void Draw(DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj) const override;
	void DrawShadow(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj) const override;

	// ==========================================
	// 3. 空间与物理 (Transform & Physics)
	// ==========================================
	void SetPosition(const DirectX::XMFLOAT3& pos) override;
	const DirectX::XMFLOAT3& GetPosition() const override { return m_position; }

	DirectX::XMFLOAT3 GetRotation() const override { return m_Rotation; }
	void SetRotationY(float angle) { m_Rotation.y = angle; }

	AABB GetAABB() override;
	void ApplyKnockback(const DirectX::XMVECTOR& direction, float force) override;

	// ==========================================
	// 4. 状态与战斗 (State & Combat)
	// ==========================================
	void Damage(float damage, bool isMelee) override;
	float GetHP() const override;
	bool IsDead() const override;
	bool IsDestroyed() const override;
	std::uint32_t GetLevel() const override { return m_Level; }
	std::uint64_t GetBaseExperience() const override { return m_BaseExperience; }

	void ChangeState(State* pNextState) override;

	bool IsAlerted() const override { return m_bAlertedStatus; }
	void SetAlerted(bool alerted);

	// ==========================================
	// 5. 公开数据成员 (Public Variables)
	// ==========================================
	// 这些变量在状态机之间共享，保持 Public
	DirectX::XMFLOAT3 m_LastKnownPosition;    // 玩家最后出现的位置
	DirectX::XMFLOAT3 m_PersonalSearchTarget; // 个人搜索目标
	bool  m_HasLostSight;                     // 是否处于丢失视野（搜索）状态
	float m_StuckTimer = 0.0f;                // 防卡死计时器

	// 全局敌人列表
	static std::vector<EnemyTest*> g_AllEnemies;

private:
	// ==========================================
	// 私有辅助函数 (Private Methods)
	// ==========================================
	bool CanSeePlayer();

	// ==========================================
	// 组件资源 (Components)
	// ==========================================
	Animator m_Animator;

	// 静态动画资源指针
	static const Animation* g_pIdleAnim;
	static const Animation* g_pWalkAnim;
	static const Animation* g_pAttackAnim;
	static const Animation* g_pScreamAnim;
	static const Animation* g_pDyingAnim;
	static const Animation* g_pReaction_HitAnim;
	static const Animation* g_pScratchIdleAnim;
	static const Animation* g_pHitMeleeAnim;
	static const Animation* g_pHitBulletAnim;

	// ==========================================
	// 属性与数值 (Attributes)
	// ==========================================
	// 空间属性
	DirectX::XMFLOAT3 m_position{};               // 位置
	DirectX::XMFLOAT3 m_Rotation = { 0.0f, 0.0f, 0.0f }; // 旋转角度（弧度）

	// 击退相关
	DirectX::XMFLOAT3 m_KnockbackVelocity = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_PendingKnockback = { 0, 0, 0 };
	float m_KnockbackDelayTimer = 0.0f;

	// 战斗属性
	float m_HP = 100.0f;
	std::uint32_t m_Level = 1;
	std::uint64_t m_BaseExperience = 25;
	float m_AttackRadius = 1.2f;      // 攻击半径
	float m_AttackCooldown = 1.0f;    // 攻击间隔（秒）
	double m_LastAttackTimer = 0.0;   // 攻击计时器

	// AI 感知属性
	float m_DetectionRadius = 8.0f;   // 探测半径
	float m_DetectionAngle = 5.0f;    // 探测角度（度）- 精确
	float m_FOVAngle = 90.0f;         // 视野总角度（度）- 扇形

	// 状态标志与计时器
	bool  m_bAlertedStatus = false;
	bool  m_bIsDead = false;          // 逻辑死亡标记 (HP<=0 但尸体还在)
	float m_DeathTimer = 0.0f;        // 尸体存在倒计时
	float m_HitAnimCooldown = 0.0f;   // 受击动画冷却时间

	// ==========================================
	// 内部状态类定义 (Inner State Classes)
	// ==========================================

	// --- 1. 巡逻状态 ---
	class EnemyTest_StatePatrol : public State {
	public:
		EnemyTest_StatePatrol(EnemyTest* pOwner);
		void Update(double elapsed_time) override;
		void Draw() const override;

	private:
		EnemyTest* m_pOwner = {};

		void PickRandomTarget();

		DirectX::XMFLOAT3 m_PatrolOrigin;     // 巡逻中心
		DirectX::XMFLOAT3 m_TargetPoint;      // 当前目标点
		float m_WaitTimer = 0.0f;             // 当前已等待时间
		const float WAIT_DURATION = 2.0f;     // 停顿观察的总时长
		const float MAX_WANDER_RADIUS = 8.0f; // 随机巡逻半径
		bool m_bAlerted = false;
	};

	// --- 2. 追逐状态 ---
	class EnemyTest_StateChase : public State {
	public:
		EnemyTest_StateChase(EnemyTest* pOwner);
		EnemyTest_StateChase(EnemyTest* pOwner, const DirectX::XMFLOAT3& targetPos);
		void Update(double elapsed_time) override;
		void Draw() const override;

	private:
		EnemyTest* m_pOwner = {};
		bool  m_HasDealtDamageInThisCycle = false;
		float m_RePathTimer = 0.0f;
		float m_GiveUpTimer;
	};

	// --- 3. 搜索状态 ---
	class EnemyTest_StateSearch : public State {
	public:
		EnemyTest_StateSearch(EnemyTest* pOwner);
		void Update(double elapsed_time) override;
		void Draw() const override {} // 不需要额外绘制

	private:
		EnemyTest* m_pOwner;
		float m_SearchTimer; // 搜索动作持续时间
	};

	// --- 4. 受击/硬直状态 ---
	class EnemyTest_StateHit : public State {
	public:
		EnemyTest_StateHit(EnemyTest* pOwner);
		void Update(double elapsed_time) override;
		void Draw() const override {}

	private:
		EnemyTest* m_pOwner;
		float m_StunTimer; // 硬直计时器
	};
};
