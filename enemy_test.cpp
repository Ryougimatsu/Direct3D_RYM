#include "enemy_test.h"
#include "collision.h"
#include "cube.h"
#include "shader_3d.h"
#include "Meshfield.h"
#include "PlayerCharacter.h"
#include <algorithm>
#include <cmath> 
#include <cstdlib>
#include <ctime>
#include "SkinningShader.h"
#include "Skeleton.h"
#include "Pathfinder.h"
#include "DropItem.h"
#include "game.h"
#include "score.h"
#include "Shader_Shadow.h"

using namespace DirectX;

// ======================================================================================
// 静态成员变量初始化
// ======================================================================================
const Animation* EnemyTest::g_pIdleAnim = nullptr;
const Animation* EnemyTest::g_pWalkAnim = nullptr;
const Animation* EnemyTest::g_pAttackAnim = nullptr;
const Animation* EnemyTest::g_pScreamAnim = nullptr;
const Animation* EnemyTest::g_pDyingAnim = nullptr;
const Animation* EnemyTest::g_pReaction_HitAnim = nullptr;
const Animation* EnemyTest::g_pScratchIdleAnim = nullptr;
const Animation* EnemyTest::g_pHitMeleeAnim = nullptr;
const Animation* EnemyTest::g_pHitBulletAnim = nullptr;
std::vector<EnemyTest*> EnemyTest::g_AllEnemies;

// ======================================================================================
// 匿名命名空间：内部使用的全局变量与辅助函数
// ======================================================================================
namespace
{
	SkinningModel* g_pSkinningModel = nullptr;

	// 辅助函数：获取随机浮点数
	static float GetRandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (max - min));
	}
}

// ======================================================================================
// 1. 生命周期 (Lifecycle)
// ======================================================================================
EnemyTest::EnemyTest(
	const DirectX::XMFLOAT3& position,
	std::uint32_t level,
	std::uint64_t baseExperience)
	: m_position(position),
	  m_Level(std::max<std::uint32_t>(1, level)),
	  m_BaseExperience(baseExperience)
{
	g_AllEnemies.push_back(this);

	m_DetectionAngle = 5.0f; // 设置索敌半径

	// 初始化旋转
	float randomAngle = ((float)rand() / RAND_MAX) * DirectX::XM_2PI;
	m_Rotation = { 0.0f, randomAngle, 0.0f };

	LoadAssets();
	if (g_pSkinningModel)
	{
		// 开始播放 Idle（循环）
		if (g_pIdleAnim)
		{
			m_Animator.PlayAnimation(g_pIdleAnim, true);
		}
	}
	// 设置初始状态为巡逻
	ChangeState(new EnemyTest_StatePatrol(this));
}

EnemyTest::~EnemyTest()
{
	auto it = std::remove(g_AllEnemies.begin(), g_AllEnemies.end(), this);
	g_AllEnemies.erase(it, g_AllEnemies.end());
}

void EnemyTest::LoadAssets()
{
	if (g_pSkinningModel == nullptr)
	{
		g_pSkinningModel = new SkinningModel();

		// 1. 加载基础模型（带骨骼）
		g_pSkinningModel->Load("resource/Model/Zombie/Zombie Idle1.fbx", 1.0f);

		// 2. 加载其它动作
		g_pSkinningModel->LoadAnimation("Walk", "resource/Model/Zombie/Zombie Walk.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Attack", "resource/Model/Zombie/Zombie Attack.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Scream", "resource/Model/Zombie/Zombie Scream.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Dying", "resource/Model/Zombie/Zombie Dying.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Reaction Hit", "resource/Model/Zombie/Zombie Reaction Hit.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Scratch Idle", "resource/Model/Zombie/Zombie Scratch Idle.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Hit_Melee", "resource/Model/Zombie/Zombie Reaction Hit by attacking.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Hit_Bullet", "resource/Model/Zombie/Zombie Reaction Hit by bullet.fbx", 1.0f);

		// 3. 获取动画指针
		if (g_pIdleAnim == nullptr) g_pIdleAnim = g_pSkinningModel->GetDefaultAnimation();
		if (g_pWalkAnim == nullptr)   g_pWalkAnim = g_pSkinningModel->GetAnimation("Walk");
		if (g_pAttackAnim == nullptr) g_pAttackAnim = g_pSkinningModel->GetAnimation("Attack");
		if (g_pScreamAnim == nullptr) g_pScreamAnim = g_pSkinningModel->GetAnimation("Scream");
		if (g_pDyingAnim == nullptr)  g_pDyingAnim = g_pSkinningModel->GetAnimation("Dying");
		if (g_pReaction_HitAnim == nullptr) g_pReaction_HitAnim = g_pSkinningModel->GetAnimation("Reaction Hit");
		if (g_pScratchIdleAnim == nullptr) g_pScratchIdleAnim = g_pSkinningModel->GetAnimation("Scratch Idle");
		if (g_pHitMeleeAnim == nullptr)  g_pHitMeleeAnim = g_pSkinningModel->GetAnimation("Hit_Melee");
		if (g_pHitBulletAnim == nullptr) g_pHitBulletAnim = g_pSkinningModel->GetAnimation("Hit_Bullet");
	}
}

void EnemyTest::UnloadAssets()
{
	if (g_pSkinningModel != nullptr) {
		g_pSkinningModel->Release();
		delete g_pSkinningModel;
		g_pSkinningModel = nullptr;

		// 重置所有静态指针
		g_pIdleAnim = nullptr;
		g_pWalkAnim = nullptr;
		g_pAttackAnim = nullptr;
		g_pScreamAnim = nullptr;
		g_pDyingAnim = nullptr;
		g_pReaction_HitAnim = nullptr;
		g_pScratchIdleAnim = nullptr;
		g_pHitMeleeAnim = nullptr;
		g_pHitBulletAnim = nullptr;
	}
}

// ======================================================================================
// 2. 核心循环与渲染 (Update & Draw)
// ======================================================================================
void EnemyTest::Update(double elapsed_time)
{
	float dt = (float)elapsed_time;

	// --- 物理击退延迟处理 ---
	if (m_KnockbackDelayTimer > 0.0f) {
		m_KnockbackDelayTimer -= dt;
		m_Animator.Update(elapsed_time);
		if (m_KnockbackDelayTimer <= 0.0f) {
			m_KnockbackVelocity = m_PendingKnockback;
			m_PendingKnockback = { 0,0,0 };
		}
		else {
			return;
		}
	}

	// --- 物理位移处理 ---
	XMVECTOR vKnock = XMLoadFloat3(&m_KnockbackVelocity);
	float currentSpeed = XMVectorGetX(XMVector3Length(vKnock));

	if (currentSpeed > 0.01f)
	{
		XMVECTOR vCurrentPos = XMLoadFloat3(&m_position);
		XMVECTOR vNextPos = vCurrentPos + vKnock * dt;
		XMFLOAT3 nextPosF3;
		XMStoreFloat3(&nextPosF3, vNextPos);
		AABB testBox = {
			 { nextPosF3.x - 0.5f, 0.0f, nextPosF3.z - 0.5f },
			 { nextPosF3.x + 0.5f, 2.0f, nextPosF3.z + 0.5f }
		};

		if (!Game_CheckCollisionWithWalls(testBox)) {
			m_position = nextPosF3;
		}
		else {
			m_KnockbackVelocity = { 0,0,0 };
			vKnock = XMVectorZero();
		}

		float friction = 10.0f;
		XMVECTOR vNewVel = XMVectorLerp(vKnock, XMVectorZero(), friction * dt);
		if (XMVectorGetX(XMVector3Length(vNewVel)) < 0.1f) {
			vNewVel = XMVectorZero();
		}

		XMStoreFloat3(&m_KnockbackVelocity, vNewVel);
		m_position.y = MeshField_GetHeight(m_position.x, m_position.z);
	}

	// --- 计时器更新 ---
	if (m_HitAnimCooldown > 0.0f) {
		m_HitAnimCooldown -= (float)elapsed_time;
	}

	// --- 死亡逻辑 ---
	if (m_bIsDead) {
		m_DeathTimer -= (float)elapsed_time;
		m_Animator.Update(elapsed_time);
		return;
	}

	// --- 基类更新与动画 ---
	Enemy::Update(elapsed_time);
	m_Animator.Update(elapsed_time);
}

void EnemyTest::Draw(DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj) const
{
	if (g_pSkinningModel == nullptr) return;

	// 1. 计算世界矩阵
	float modelScale = 0.01f;
	XMMATRIX S = XMMatrixScaling(modelScale, modelScale, modelScale);
	XMMATRIX R = XMMatrixRotationY(m_Rotation.y + XM_PI);
	XMMATRIX T = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX World = S * R * T;

	// 2. 准备渲染环境
	SkinningShader_3D_Begin();

	// 3. 设置矩阵
	SkinningShader_3D_SetWorldMatrix(World);
	SkinningShader_3D_SetViewMatrix(view);
	SkinningShader_3D_SetProjectMatrix(proj);

	// 4. 获取并设置骨骼变换矩阵
	std::vector<XMMATRIX> bones = const_cast<Animator&>(m_Animator).GetFinalBoneMatrices(g_pSkinningModel->GetSkeleton());
	SkinningShader_3D_SetBoneTransforms(bones);

	// 5. 绘制
	const_cast<SkinningModel*>(g_pSkinningModel)->Draw();
}

void EnemyTest::DrawShadow(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj) const
{
	if (g_pSkinningModel == nullptr) return;

	// 1. 计算世界矩阵
	float modelScale = 0.01f;
	XMMATRIX S = XMMatrixScaling(modelScale, modelScale, modelScale);
	XMMATRIX R = XMMatrixRotationY(m_Rotation.y + XM_PI);
	XMMATRIX T = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX World = S * R * T;

	// 2. 设置世界矩阵
	Shader_Shadow_SetWorldMatrix(World);

	// 3. 设置骨骼变换矩阵
	std::vector<XMMATRIX> bones = const_cast<Animator&>(m_Animator).GetFinalBoneMatrices(g_pSkinningModel->GetSkeleton());
	SkinningShader_3D_SetBoneTransforms(bones);

	// 4. 绘制
	const_cast<SkinningModel*>(g_pSkinningModel)->Draw();
}

// ======================================================================================
// 3. 空间、物理与感知 (Transform, Physics & AI)
// ======================================================================================
void EnemyTest::SetPosition(const DirectX::XMFLOAT3& pos)
{
	if (m_bIsDead) return;
	m_position = pos;
}

AABB EnemyTest::GetAABB()
{
	float hw = 0.5f; // 半宽

	// 如果敌人死了，高度变为 0.2f (变成地上的绊脚石)；活着则是 2.0f
	float h = m_bIsDead ? 0.2f : 2.0f;

	return {
		{ m_position.x - hw, m_position.y,     m_position.z - hw },
		{ m_position.x + hw, m_position.y + h, m_position.z + hw }
	};
}

void EnemyTest::ApplyKnockback(const DirectX::XMVECTOR& direction, float force)
{
	if (m_bIsDead) return;

	XMVECTOR knockDir = XMVectorSetY(direction, 0.0f);
	knockDir = XMVector3Normalize(knockDir);

	float speedMultiplier = 3.0f;

	// 直接设置速度，不要延迟
	XMStoreFloat3(&m_KnockbackVelocity, knockDir * force * speedMultiplier);

	// 将延迟设为 0，让物理在下一帧立刻生效
	m_KnockbackDelayTimer = 0.9f;
}

bool EnemyTest::CanSeePlayer()
{
	XMFLOAT3 playerPos = Player_GetPosition();
	XMFLOAT3 enemyPos = m_position;
	// 抬高视线起点，防止从脚底发射射线被地板挡住
	enemyPos.y += 1.0f;
	playerPos.y += 1.0f;

	// 1. 距离检测
	XMVECTOR vToPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&enemyPos);
	float distSq = XMVectorGetX(XMVector3LengthSq(vToPlayer));
	if (distSq > m_DetectionRadius * m_DetectionRadius) return false;

	if (m_bAlertedStatus)
	{
		if (distSq < 25.0f) {
			if (!Game_IsLineOfSightBlocked(enemyPos, playerPos)) {
				return true;
			}
		}
	}

	// 2. 角度检测
	vToPlayer = XMVector3Normalize(vToPlayer);
	XMVECTOR vEnemyForward = XMVectorSet(sinf(m_Rotation.y), 0.0f, cosf(m_Rotation.y), 0.0f);
	float dot = XMVectorGetX(XMVector3Dot(vEnemyForward, vToPlayer));
	float halfFOV = XMConvertToRadians(m_FOVAngle * 0.5f);
	if (dot < cosf(halfFOV)) return false;

	// 3. 射线遮挡检测
	if (Game_IsLineOfSightBlocked(enemyPos, playerPos)) {
		return false;
	}

	return true;
}

// ======================================================================================
// 4. 战斗与状态管理 (Combat & State)
// ======================================================================================
void EnemyTest::Damage(float damage, bool isMelee)
{
	if (m_bIsDead) return;

	// 1. 通用逻辑：警觉
	m_bAlertedStatus = true;
	XMFLOAT3 playerPos = Player_GetPosition();
	m_LastKnownPosition = playerPos;
	m_PersonalSearchTarget = playerPos;
	m_HasLostSight = false;

	// 2. 扣血与死亡判定
	m_HP -= damage;

	if (m_HP <= 0.0f) {
		Score_AddScore(100);
		m_HP = 0.0f;
		m_bIsDead = true;

		// This death transition runs only once, so EXP cannot be granted twice.
		Enemy_AwardDefeatExperience(*this);

		m_Animator.PlayAnimation(g_pDyingAnim, false, 0.2f);
		m_DeathTimer = 3.5f;
		m_KnockbackVelocity = { 0,0,0 }; // 死亡时消除所有速度

		int rate = rand() % 100;
		if (rate < 15) {
			DropItem_Spawn(m_position, 4);
		}
		return;
	}

	// 3. 受击分歧
	if (isMelee)
	{
		// A. 近战逻辑
		ChangeState(new EnemyTest_StateHit(this));
		m_Path.clear();
		m_CurrentPathIndex = 0;
		m_HitAnimCooldown = 1.0f;

		if (g_pHitMeleeAnim) {
			m_Animator.PlayAnimation(g_pHitMeleeAnim, false, 0.02f);
			m_Animator.SetSpeedScale(0.8f);
		}

		XMVECTOR vToEnemy = XMLoadFloat3(&m_position) - XMLoadFloat3(&playerPos);
		ApplyKnockback(vToEnemy, 4.0f);
	}
	else
	{
		// B. 子弹逻辑
		if (m_HitAnimCooldown <= 0.0f)
		{
			if (g_pHitBulletAnim) {
				m_Animator.PlayAnimation(g_pHitBulletAnim, false, 0.1f);
				m_Animator.SetSpeedScale(1.2f);
			}
			m_HitAnimCooldown = 0.4f;
		}
	}
}

float EnemyTest::GetHP() const
{
	return m_HP;
}

void EnemyTest::ApplyDifficultyScaling(
	float hpMultiplier,
	float damageMultiplier,
	float speedMultiplier)
{
	constexpr float MIN_HP = 1.0f;
	constexpr float MIN_DAMAGE = 1.0f;
	constexpr float MIN_SPEED = 0.01f;

	m_MaxHP = std::max(MIN_HP, m_MaxHP * hpMultiplier);
	m_HP = m_MaxHP;
	m_AttackDamage = std::max(MIN_DAMAGE, m_AttackDamage * damageMultiplier);
	m_MoveSpeed = std::max(MIN_SPEED, m_MoveSpeed * speedMultiplier);
}

bool EnemyTest::IsDead() const
{
	return m_HP <= 0.0f;
}

void EnemyTest::ChangeState(State* pNextState)
{
	Enemy::ChangeState(pNextState);
}

void EnemyTest::SetAlerted(bool alerted) { m_bAlertedStatus = alerted; }

bool EnemyTest::IsDestroyed() const
{
	return m_bIsDead && (m_DeathTimer <= 0.0f);
}


// ======================================================================================
// 5. 状态机类实现：巡逻 (StatePatrol)
// ======================================================================================
EnemyTest::EnemyTest_StatePatrol::EnemyTest_StatePatrol(EnemyTest* pOwner)
	: m_pOwner(pOwner)
{
	m_PatrolOrigin = pOwner->m_position; // 记住出生点
	PickRandomTarget();
}

void EnemyTest::EnemyTest_StatePatrol::PickRandomTarget()
{
	const int MAX_ATTEMPTS = 20;

	for (int i = 0; i < MAX_ATTEMPTS; ++i)
	{
		float randX = m_PatrolOrigin.x + GetRandomFloat(-MAX_WANDER_RADIUS, MAX_WANDER_RADIUS);
		float randZ = m_PatrolOrigin.z + GetRandomFloat(-MAX_WANDER_RADIUS, MAX_WANDER_RADIUS);

		float halfSize = 0.5f;
		AABB testBox = {
			{ randX - halfSize, 0.0f, randZ - halfSize },
			{ randX + halfSize, 2.0f, randZ + halfSize }
		};

		if (!Game_CheckCollisionWithWalls(testBox))
		{
			m_TargetPoint.x = randX;
			m_TargetPoint.z = randZ;
			m_TargetPoint.y = MeshField_GetHeight(m_TargetPoint.x, m_TargetPoint.z);
			return;
		}
	}

	m_TargetPoint = m_PatrolOrigin;
	m_TargetPoint.y = MeshField_GetHeight(m_TargetPoint.x, m_TargetPoint.z);
}

void EnemyTest::EnemyTest_StatePatrol::Update(double elapsed_time)
{
	if (m_pOwner->CanSeePlayer())
	{
		m_pOwner->SetAlerted(true);
		m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateChase(m_pOwner));
		return;
	}

	XMFLOAT3 soundPos;
	float soundRadius;
	if (Sound_GetLatest(soundPos, soundRadius)) {
		float effectiveRadius = soundRadius * 0.7f;

		XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position);
		XMVECTOR vSoundPos = XMLoadFloat3(&soundPos);
		XMVECTOR toSound = vSoundPos - vEnemyPos;
		float distSq = XMVectorGetX(XMVector3LengthSq(toSound));

		if (distSq < (effectiveRadius * effectiveRadius)) {
			m_TargetPoint = soundPos;

			// A: 还没警觉 -> 尖叫
			if (!m_bAlerted) {
				if (!m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
					m_pOwner->m_Animator.PlayAnimation(g_pScreamAnim, false, 0.2f);
					m_bAlerted = true;
					m_pOwner->SetAlerted(true);
				}
				return;
			}

			// B: 正在尖叫中
			if (m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
				return;
			}

			// C: 警觉且没尖叫 -> 调查
			m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateChase(m_pOwner, soundPos));
			return;
		}
	}

	if (m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
		if (m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.9f) {
			return;
		}
		if (m_bAlerted) {
			m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateChase(m_pOwner, m_TargetPoint));
			return;
		}
	}

	if (m_WaitTimer > 0.0f)
	{
		m_WaitTimer -= (float)elapsed_time;
		if (g_pIdleAnim) m_pOwner->m_Animator.PlayAnimation(g_pIdleAnim, true, 0.5f);
		return;
	}

	// --- 移动逻辑 ---
	XMVECTOR vPos = XMLoadFloat3(&m_pOwner->m_position);
	XMVECTOR vTarget = XMLoadFloat3(&m_TargetPoint);
	XMVECTOR toTarget = vTarget - vPos;
	toTarget = XMVectorSetY(toTarget, 0.0f);

	float distToTarget = XMVectorGetX(XMVector3Length(toTarget));
	if (distToTarget > 0.1f)
	{
		if (g_pWalkAnim) m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.8f);

		XMVECTOR dir = XMVector3Normalize(toTarget);
		float patrolSpeed = 0.8f;
		XMFLOAT3 oldPos = m_pOwner->m_position;

		XMVECTOR vNewPos = vPos + dir * patrolSpeed * (float)elapsed_time;
		XMStoreFloat3(&m_pOwner->m_position, vNewPos);

		if (Game_CheckCollisionWithWalls(m_pOwner->GetAABB()))
		{
			m_pOwner->m_position = oldPos;
			PickRandomTarget();
		}

		float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
		m_pOwner->SetRotationY(angle);
		m_pOwner->m_Animator.SetSpeedScale(patrolSpeed / 1.0f);
	}
	else
	{
		m_WaitTimer = WAIT_DURATION;
		PickRandomTarget();
		m_bAlerted = false;
	}
	m_pOwner->m_Animator.SetSpeedScale(1.0f);
	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
}

void EnemyTest::EnemyTest_StatePatrol::Draw() const {}


// ======================================================================================
// 6. 状态机类实现：追逐 (StateChase)
// ======================================================================================
EnemyTest::EnemyTest_StateChase::EnemyTest_StateChase(EnemyTest* pOwner)
	: m_pOwner(pOwner), m_RePathTimer(0.0f), m_GiveUpTimer(0.0f)
{
	if (g_pWalkAnim) {
		m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.3f);
	}
}

EnemyTest::EnemyTest_StateChase::EnemyTest_StateChase(EnemyTest* pOwner, const DirectX::XMFLOAT3& targetPos)
	: m_pOwner(pOwner), m_RePathTimer(0.0f), m_GiveUpTimer(0.0f)
{
	if (g_pWalkAnim) {
		m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.3f);
	}

	m_pOwner->m_HasLostSight = true;
	m_pOwner->m_PersonalSearchTarget = targetPos;
	m_pOwner->m_LastKnownPosition = targetPos;
}

void EnemyTest::EnemyTest_StateChase::Update(double elapsed_time)
{
	// --- 0. 预处理 ---
	if (m_pOwner->m_StuckTimer > 0.0f) {
		m_pOwner->m_StuckTimer -= (float)elapsed_time;
	}

	XMFLOAT3 playerPos = Player_GetPosition();
	XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position);
	XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);

	if (m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
		if (m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.9f) {
			m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
			return;
		}
	}

	// --- 1. 视野判定 & 目标锁定 ---
	bool canSee = m_pOwner->CanSeePlayer();
	float distToPlayer = XMVectorGetX(XMVector3Length(XMVectorSetY(vPlayerPos - vEnemyPos, 0.0f)));
	if (distToPlayer < 1.5f) canSee = true;

	XMFLOAT3 targetPos;

	if (canSee) {
		m_pOwner->m_LastKnownPosition = playerPos;
		m_pOwner->m_HasLostSight = false;
		m_pOwner->m_PersonalSearchTarget = playerPos;
		targetPos = playerPos;
	}
	else {
		// 没看见玩家，监听声音
		XMFLOAT3 soundPos;
		float soundRadius;
		if (Sound_GetLatest(soundPos, soundRadius)) {
			XMVECTOR vSound = XMLoadFloat3(&soundPos);
			XMVECTOR vCurrentTarget = XMLoadFloat3(&m_pOwner->m_PersonalSearchTarget);
			if (XMVectorGetX(XMVector3LengthSq(vSound - vCurrentTarget)) > 4.0f) {
				m_pOwner->m_PersonalSearchTarget = soundPos;
				m_pOwner->m_CurrentPathIndex = 0;
				m_pOwner->m_Path.clear();
			}
		}

		// 丢失视野搜索逻辑
		if (!m_pOwner->m_HasLostSight)
		{
			XMFLOAT3 basePos = m_pOwner->m_LastKnownPosition;
			bool foundValidSpot = false;

			for (int i = 0; i < 10; ++i)
			{
				float randomAngle = (rand() % 360) * XM_2PI / 360.0f;
				float randomDist = 1.25f + (rand() % 200) / 100.0f;

				XMFLOAT3 testPos;
				testPos.x = basePos.x + sinf(randomAngle) * randomDist;
				testPos.z = basePos.z + cosf(randomAngle) * randomDist;
				testPos.y = basePos.y;

				AABB testBox = {
					{testPos.x - 0.4f, testPos.y, testPos.z - 0.4f},
					{testPos.x + 0.4f, testPos.y + 1.0f, testPos.z + 0.4f}
				};

				if (!Game_CheckCollisionWithWalls(testBox)) {
					bool tooCloseToOthers = false;
					for (auto* other : EnemyTest::g_AllEnemies) {
						if (other == m_pOwner) continue;
						if (other->m_HasLostSight) {
							XMVECTOR v1 = XMLoadFloat3(&testPos);
							XMVECTOR v2 = XMLoadFloat3(&other->m_PersonalSearchTarget);
							if (XMVectorGetX(XMVector3LengthSq(v1 - v2)) < 4.0f) {
								tooCloseToOthers = true;
								break;
							}
						}
					}
					if (!tooCloseToOthers || i == 9) {
						m_pOwner->m_PersonalSearchTarget = testPos;
						foundValidSpot = true;
						break;
					}
				}
			}
			if (!foundValidSpot) {
				m_pOwner->m_PersonalSearchTarget = basePos;
			}
		}
		m_pOwner->m_HasLostSight = true;
		targetPos = m_pOwner->m_PersonalSearchTarget;
	}

	// --- 2. 检查到达搜索点 ---
	if (m_pOwner->m_HasLostSight)
	{
		XMVECTOR vTarget = XMLoadFloat3(&targetPos);
		float distToTarget = XMVectorGetX(XMVector3Length(XMVectorSetY(vTarget - vEnemyPos, 0.0f)));

		bool hasArrived = (distToTarget < 0.6f);
		if (!m_pOwner->m_Path.empty() && m_pOwner->m_CurrentPathIndex >= m_pOwner->m_Path.size()) {
			hasArrived = true;
		}

		if (hasArrived) {
			m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateSearch(m_pOwner));
			return;
		}
	}

	// 物理击退时限制移动
	XMVECTOR vKnockVel = XMLoadFloat3(&m_pOwner->m_KnockbackVelocity);
	if (XMVectorGetX(XMVector3LengthSq(vKnockVel)) > 0.01f)
	{
		m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
		XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);
		if (distToPlayer > 0.1f) {
			XMVECTOR dir = XMVector3Normalize(XMVectorSetY(vPlayerPos - vEnemyPos, 0.0f));
			float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
			m_pOwner->SetRotationY(angle);
		}
		return;
	}

	// --- 3. 攻击逻辑 ---
	bool isPlayingAttack = m_pOwner->m_Animator.IsPlaying(g_pAttackAnim);
	bool inAttackRange = (distToPlayer < m_pOwner->m_AttackRadius + 0.35f);

	if (!m_pOwner->m_HasLostSight)
	{
		if (inAttackRange || (isPlayingAttack && m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.95f))
		{
			m_pOwner->m_Animator.PlayAnimation(g_pAttackAnim, true, 0.1f);
			m_pOwner->m_Animator.SetSpeedScale(1.2f);

			float progress = m_pOwner->m_Animator.GetCurrentAnimationProgress();
			if (progress < 0.2f) m_HasDealtDamageInThisCycle = false;
			if (progress > 0.3f && !m_HasDealtDamageInThisCycle) {
				if (distToPlayer < m_pOwner->m_AttackRadius + 1.0f) {
					Player_Damage(m_pOwner->m_AttackDamage);
				}
				m_HasDealtDamageInThisCycle = true;
			}

			if (distToPlayer > 0.1f) {
				XMVECTOR dir = XMVector3Normalize(XMVectorSetY(vPlayerPos - vEnemyPos, 0.0f));
				float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
				m_pOwner->SetRotationY(angle);
			}
			return;
		}
	}

	// --- 4. 移动逻辑 ---
	if (!isPlayingAttack)
	{
		m_pOwner->MoveToTarget(targetPos, elapsed_time);

		// 旋转计算
		XMVECTOR vLookTarget = XMLoadFloat3(&targetPos);
		if (!m_pOwner->m_Path.empty() && m_pOwner->m_CurrentPathIndex < m_pOwner->m_Path.size())
		{
			XMFLOAT3 currentPt = m_pOwner->m_Path[m_pOwner->m_CurrentPathIndex];
			XMVECTOR vCurrentPt = XMLoadFloat3(&currentPt);
			float distToCurrentPt = XMVectorGetX(XMVector3Length(XMVectorSetY(vCurrentPt - vEnemyPos, 0.0f)));

			if (distToCurrentPt < 1.0f && (m_pOwner->m_CurrentPathIndex + 1 < m_pOwner->m_Path.size()))
			{
				XMFLOAT3 nextPt = m_pOwner->m_Path[m_pOwner->m_CurrentPathIndex + 1];
				vLookTarget = XMLoadFloat3(&nextPt);
			}
			else
			{
				vLookTarget = vCurrentPt;
			}
		}

		XMVECTOR vDir = XMVectorSetY(vLookTarget - vEnemyPos, 0.0f);
		float distSq = XMVectorGetX(XMVector3LengthSq(vDir));

		if (distSq > 0.01f)
		{
			XMVECTOR dirNorm = XMVector3Normalize(vDir);
			float targetAngle = atan2f(XMVectorGetX(dirNorm), XMVectorGetZ(dirNorm));
			float currentAngle = m_pOwner->GetRotation().y;
			float diff = targetAngle - currentAngle;
			while (diff > XM_PI) diff -= XM_2PI;
			while (diff < -XM_PI) diff += XM_2PI;
			float smoothAngle = currentAngle + diff * 5.0f * (float)elapsed_time;
			m_pOwner->SetRotationY(smoothAngle);
		}

		// 动画控制
		bool isGettingShot = false;
		if (g_pHitBulletAnim && m_pOwner->m_Animator.IsPlaying(g_pHitBulletAnim)) {
			if (m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.95f) {
				isGettingShot = true;
			}
		}

		if (!isGettingShot)
		{
			if (!m_pOwner->m_Animator.IsPlaying(g_pWalkAnim)) {
				if (g_pWalkAnim) m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.2f);
				m_pOwner->m_Animator.SetSpeedScale(1.0f);
			}
		}
	}
	else {
		// 停下来了
		if (!m_pOwner->m_Animator.IsPlaying(g_pIdleAnim)) {
			if (g_pIdleAnim) m_pOwner->m_Animator.PlayAnimation(g_pIdleAnim, true, 0.2f);
			m_pOwner->m_Animator.SetSpeedScale(1.0f);
		}
	}

	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
}

void EnemyTest::EnemyTest_StateChase::Draw() const {}


// ======================================================================================
// 7. 状态机类实现：搜索 (StateSearch)
// ======================================================================================
EnemyTest::EnemyTest_StateSearch::EnemyTest_StateSearch(EnemyTest* pOwner)
	: m_pOwner(pOwner), m_SearchTimer(2.5f)
{
	if (g_pScratchIdleAnim) {
		m_pOwner->m_Animator.PlayAnimation(g_pScratchIdleAnim, true, 0.2f);
	}
	else if (g_pIdleAnim) {
		m_pOwner->m_Animator.PlayAnimation(g_pIdleAnim, true, 0.2f);
	}
}

void EnemyTest::EnemyTest_StateSearch::Update(double elapsed_time)
{
	if (m_pOwner->CanSeePlayer())
	{
		m_pOwner->SetAlerted(true);
		m_pOwner->ChangeState(new EnemyTest_StateChase(m_pOwner));
		return;
	}

	m_SearchTimer -= (float)elapsed_time;

	if (m_SearchTimer <= 0.0f)
	{
		m_pOwner->SetAlerted(false);
		m_pOwner->ChangeState(new EnemyTest_StatePatrol(m_pOwner));
		return;
	}

	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
}


// ======================================================================================
// 8. 状态机类实现：受击硬直 (StateHit)
// ======================================================================================
EnemyTest::EnemyTest_StateHit::EnemyTest_StateHit(EnemyTest* pOwner)
	: m_pOwner(pOwner), m_StunTimer(0.0f)
{
	m_StunTimer = m_pOwner->m_HitAnimCooldown;
	if (m_StunTimer <= 0.0f) m_StunTimer = 0.5f;
}

void EnemyTest::EnemyTest_StateHit::Update(double elapsed_time)
{
	m_StunTimer -= (float)elapsed_time;
	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);

	XMVECTOR vKnock = XMLoadFloat3(&m_pOwner->m_KnockbackVelocity);
	float currentSpeed = XMVectorGetX(XMVector3LengthSq(vKnock));
	bool isStopped = (currentSpeed < 0.01f);

	bool isHitAnimPlaying = false;
	if (g_pHitMeleeAnim && m_pOwner->m_Animator.IsPlaying(g_pHitMeleeAnim)) isHitAnimPlaying = true;
	if (g_pHitBulletAnim && m_pOwner->m_Animator.IsPlaying(g_pHitBulletAnim)) isHitAnimPlaying = true;

	bool isAnimFinished = true;
	if (isHitAnimPlaying) {
		if (m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.9f) {
			isAnimFinished = false;
		}
	}
	bool isStunTimerOver = (m_StunTimer <= 0.0f);

	if (isStopped && isStunTimerOver && isAnimFinished)
	{
		m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateChase(m_pOwner));
		return;
	}
}
