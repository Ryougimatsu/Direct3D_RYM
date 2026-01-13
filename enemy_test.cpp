#include "enemy_test.h"
#include "collision.h"
#include "cube.h"
#include "shader_3d.h"
#include "Meshfield.h"
#include "PlayerCharacter.h"
#include <cmath> 
#include <cstdlib>
#include <ctime>
#include "SkinningShader.h"
#include "Skeleton.h"
#include "Pathfinder.h"
#include "DropItem.h"
#include "game.h"
#include "score.h"
using namespace DirectX;


const Animation* EnemyTest::g_pIdleAnim = nullptr;
const Animation* EnemyTest::g_pWalkAnim = nullptr;
const Animation* EnemyTest::g_pAttackAnim = nullptr;
const Animation* EnemyTest::g_pScreamAnim = nullptr;
const Animation* EnemyTest::g_pDyingAnim = nullptr;
const Animation* EnemyTest::g_pReaction_HitAnim = nullptr;
const Animation* EnemyTest::g_pScratchIdleAnim = nullptr;
std::vector<EnemyTest*> EnemyTest::g_AllEnemies;
namespace 
{
	SkinningModel* g_pSkinningModel = nullptr;
	
}
// ========================================================
// 状态机逻辑实现
// ========================================================

static float GetRandomFloat(float min, float max) {
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (max - min));
}

void EnemyTest::EnemyTest_StatePatrol::PickRandomTarget()
{
	// 设置最大尝试次数，防止地图全被填满时陷入死循环
	const int MAX_ATTEMPTS = 20;

	for (int i = 0; i < MAX_ATTEMPTS; ++i)
	{
		// 1. 在中心点周围随机生成 X 和 Z
		float randX = m_PatrolOrigin.x + GetRandomFloat(-MAX_WANDER_RADIUS, MAX_WANDER_RADIUS);
		float randZ = m_PatrolOrigin.z + GetRandomFloat(-MAX_WANDER_RADIUS, MAX_WANDER_RADIUS);

		// 2. 【核心修改】检查这个点是否在箱子里
		// 我们在这个随机点构造一个虚拟的 AABB (大小和敌人差不多，比如 1x1)
		float halfSize = 0.5f;
		AABB testBox = {
			{ randX - halfSize, 0.0f, randZ - halfSize }, // Min
			{ randX + halfSize, 2.0f, randZ + halfSize }  // Max
		};

		// 3. 调用碰撞检测
		if (!Game_CheckCollisionWithWalls(testBox))
		{
			// 如果没撞墙，说明这个点是安全的，应用它并退出循环
			m_TargetPoint.x = randX;
			m_TargetPoint.z = randZ;
			m_TargetPoint.y = MeshField_GetHeight(m_TargetPoint.x, m_TargetPoint.z);
			return;
		}

		// 如果撞墙了 (CheckCollision 返回 true)，循环继续，尝试下一次生成...
	}

	// 4. 如果尝试了 20 次都失败了 (太倒霉了或者被围住了)，就只能原地待命
	// 或者设回出生点，防止敌人跑去非法区域
	m_TargetPoint = m_PatrolOrigin;
	m_TargetPoint.y = MeshField_GetHeight(m_TargetPoint.x, m_TargetPoint.z);
}

EnemyTest::EnemyTest_StatePatrol::EnemyTest_StatePatrol(EnemyTest* pOwner)
	: m_pOwner(pOwner)
{

	m_PatrolOrigin = pOwner->m_position; // 记住出生点
	PickRandomTarget();
}

void EnemyTest::EnemyTest_StatePatrol::Update(double elapsed_time)
{


	if (m_pOwner->CanSeePlayer())
	{
		// 1. 标记为警觉状态
		m_pOwner->SetAlerted(true);

		// 2. 切换到追逐状态 (StateChase)
		m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateChase(m_pOwner));

		return;
	}

	XMFLOAT3 soundPos;
	float soundRadius;
	if (Sound_GetLatest(soundPos, soundRadius)) {
		// 【修正】在这里计算有效半径
		float effectiveRadius = soundRadius * 0.7f;

		XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position);
		XMVECTOR vSoundPos = XMLoadFloat3(&soundPos);
		XMVECTOR toSound = vSoundPos - vEnemyPos;
		float distSq = XMVectorGetX(XMVector3LengthSq(toSound));

		if (distSq < (effectiveRadius * effectiveRadius)) {
			// 始终更新目标点，以防声音移动
			m_TargetPoint = soundPos;

			// 情况 A: 还没警觉 -> 吓一跳，开始尖叫
			if (!m_bAlerted) {
				if (!m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
					m_pOwner->m_Animator.PlayAnimation(g_pScreamAnim, false, 0.2f);
					m_bAlerted = true;
					m_pOwner->SetAlerted(true);
				}
				return; // 正在尖叫中，直接返回，不切状态
			}

			// 情况 B: 已经警觉了 (m_bAlerted == true)

			if (m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
				return;
			}

			// 情况 C: 警觉了，且没在尖叫（比如之前的尖叫播完了，又听到了新声音）
			// 这时候才允许直接切换去调查
			m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateChase(m_pOwner, soundPos));
			return;
		}
	}

	if (m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
		// 如果尖叫进度没到 90%，直接返回，不执行下面的位移代码
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
	toTarget = XMVectorSetY(toTarget, 0.0f); // 忽略高度

	float distToTarget = XMVectorGetX(XMVector3Length(toTarget));
	if (distToTarget > 0.1f) // 还没走到
	{
		if (g_pWalkAnim) m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.8f);

		XMVECTOR dir = XMVector3Normalize(toTarget);
		float patrolSpeed = 0.8f;

		// 1. 备份旧位置
		XMFLOAT3 oldPos = m_pOwner->m_position;

		// 2. 尝试移动
		XMVECTOR vNewPos = vPos + dir * patrolSpeed * (float)elapsed_time;
		XMStoreFloat3(&m_pOwner->m_position, vNewPos);

		// 3. 【新增】碰撞检测
		if (Game_CheckCollisionWithWalls(m_pOwner->GetAABB()))
		{
			// 撞墙了！
			// A. 回退到撞墙前的位置
			m_pOwner->m_position = oldPos;

			// B. 此路不通，立刻重新选一个随机目标点 (这样敌人就会转身走开)
			PickRandomTarget();
		}

		// 更新朝向
		float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
		m_pOwner->SetRotationY(angle);
		m_pOwner->m_Animator.SetSpeedScale(patrolSpeed / 1.0f);
	}
	else
	{
		// 到达随机目标点，进入等待，并生成下一个目标点
		m_WaitTimer = WAIT_DURATION;
		PickRandomTarget();
		m_bAlerted = false;
	}
	m_pOwner->m_Animator.SetSpeedScale(1.0f);


	// 维持原有的地形适配逻辑
	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
}


void EnemyTest::EnemyTest_StatePatrol::Draw() const
{

}
// ========================================================
// 状态机 - 追逐
// ========================================================
EnemyTest::EnemyTest_StateChase::EnemyTest_StateChase(EnemyTest* pOwner)
	: m_pOwner(pOwner), m_RePathTimer(0.0f),m_GiveUpTimer(0.0f)
{
	// 切换到追逐状态时，播放移动动画
	if (g_pWalkAnim) {
		m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.3f); // 0.3秒平滑过渡
	}
}
EnemyTest::EnemyTest_StateChase::EnemyTest_StateChase(EnemyTest* pOwner, const DirectX::XMFLOAT3& targetPos)
	: m_pOwner(pOwner), m_RePathTimer(0.0f), m_GiveUpTimer(0.0f)
{
	// 1. 播放移动动画
	if (g_pWalkAnim) {
		m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.3f);
	}

	// 2. 欺骗状态机，让它认为“刚刚丢失了玩家视野”
	m_pOwner->m_HasLostSight = true;

	// 3. 将声音位置设定为搜索目标
	m_pOwner->m_PersonalSearchTarget = targetPos;

	// 4. 更新最后已知位置（防止逻辑出错）
	m_pOwner->m_LastKnownPosition = targetPos;
}
void EnemyTest::EnemyTest_StateChase::Update(double elapsed_time)
{
	// =================================================================================
	// 0. 预处理
	// =================================================================================
	if (m_pOwner->m_StuckTimer > 0.0f) {
		m_pOwner->m_StuckTimer -= (float)elapsed_time;
	}

	XMFLOAT3 playerPos = Player_GetPosition();
	XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position);
	XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);

	// 尖叫硬直
	if (m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
		if (m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.9f) {
			m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
			return;
		}
	}

	// =================================================================================
	// 1. 视野判定 & 目标锁定
	// =================================================================================
	bool canSee = m_pOwner->CanSeePlayer();

	// 距离检测：算出与玩家的真实距离
	float distToPlayer = XMVectorGetX(XMVector3Length(XMVectorSetY(vPlayerPos - vEnemyPos, 0.0f)));

	// 如果距离小于 1.5米，就算射线被挡住也视为“看见”
	if (distToPlayer < 1.5f) canSee = true;

	XMFLOAT3 targetPos;

	if (canSee) {
		m_pOwner->m_LastKnownPosition = playerPos;
		m_pOwner->m_HasLostSight = false;
		m_pOwner->m_PersonalSearchTarget = playerPos;
		targetPos = playerPos;
	}
	else {
		// 丢失视野逻辑 (保持不变)
		if (!m_pOwner->m_HasLostSight)
		{
			XMFLOAT3 basePos = m_pOwner->m_LastKnownPosition;

			// 尝试生成一个不在墙里的搜索点
			bool foundValidSpot = false;

			// 【优化 1】增加尝试次数，确保能找到合适的位置
			for (int i = 0; i < 10; ++i)
			{
				float randomAngle = (rand() % 360) * XM_2PI / 360.0f;

				// 搜索半径
				float randomDist = 1.25f + (rand() % 200) / 100.0f;

				XMFLOAT3 testPos;
				testPos.x = basePos.x + sinf(randomAngle) * randomDist;
				testPos.z = basePos.z + cosf(randomAngle) * randomDist;
				testPos.y = basePos.y;

				// 构造一个小盒子检测这个点是不是墙
				AABB testBox = {
					{testPos.x - 0.4f, testPos.y, testPos.z - 0.4f},
					{testPos.x + 0.4f, testPos.y + 1.0f, testPos.z + 0.4f}
				};

				// 确保这个点不在墙里
				if (!Game_CheckCollisionWithWalls(testBox)) {

					// 检查这个点是否和其他敌人的目标点太近，避免“撞车”
					bool tooCloseToOthers = false;
					for (auto* other : EnemyTest::g_AllEnemies) {
						if (other == m_pOwner) continue;
						// 如果其他敌人也在搜索，且它的目标点和我的很像
						if (other->m_HasLostSight) {
							XMVECTOR v1 = XMLoadFloat3(&testPos);
							XMVECTOR v2 = XMLoadFloat3(&other->m_PersonalSearchTarget);
							if (XMVectorGetX(XMVector3LengthSq(v1 - v2)) < 2.0f * 2.0f) { // 2米内有人了
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

	// =================================================================================
	// 2. 检查到达搜索点 (保持不变)
	// =================================================================================
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

	// =================================================================================
	// 3. 攻击逻辑
	// =================================================================================
	bool isPlayingAttack = m_pOwner->m_Animator.IsPlaying(g_pAttackAnim);


	bool inAttackRange = (distToPlayer < m_pOwner->m_AttackRadius + 0.35f);

	if (!m_pOwner->m_HasLostSight)
	{
		if (inAttackRange || (isPlayingAttack && m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.95f))
		{
			// 播放攻击动画
			m_pOwner->m_Animator.PlayAnimation(g_pAttackAnim, true, 0.1f);
			m_pOwner->m_Animator.SetSpeedScale(1.2f);

			// 伤害判定 (保持你原有的逻辑)
			float progress = m_pOwner->m_Animator.GetCurrentAnimationProgress();
			if (progress < 0.2f) m_HasDealtDamageInThisCycle = false;
			if (progress > 0.3f && !m_HasDealtDamageInThisCycle) {
				if (distToPlayer < m_pOwner->m_AttackRadius + 1.0f) { // 伤害判定也稍微宽容点
					Player_Damage(10.0f);
				}
				m_HasDealtDamageInThisCycle = true;
			}

			// 
			// 也要加防鬼畜保护：只有距离大于 0.1 才更新朝向
			if (distToPlayer > 0.1f) {
				XMVECTOR dir = XMVector3Normalize(XMVectorSetY(vPlayerPos - vEnemyPos, 0.0f));
				float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
				m_pOwner->SetRotationY(angle);
			}

			return; // 攻击中不移动
		}
	}

	// =================================================================================
	// 4. 移动逻辑
	// =================================================================================
	if (!isPlayingAttack)
	{
		// 核心移动
		m_pOwner->MoveToTarget(targetPos, elapsed_time);

		// 1. 默认看最终目标 
		XMVECTOR vLookTarget = XMLoadFloat3(&targetPos);

		// 2. 如果有路径，优先看路点
		if (!m_pOwner->m_Path.empty() && m_pOwner->m_CurrentPathIndex < m_pOwner->m_Path.size())
		{
			// 获取当前要去路点
			XMFLOAT3 currentPt = m_pOwner->m_Path[m_pOwner->m_CurrentPathIndex];
			XMVECTOR vCurrentPt = XMLoadFloat3(&currentPt);

			// 算出离当前路点的距离
			float distToCurrentPt = XMVectorGetX(XMVector3Length(XMVectorSetY(vCurrentPt - vEnemyPos, 0.0f)));

			// 如果离当前拐角很近了 (< 1.0米)，且后面还有路，就提前看下一个点
			if (distToCurrentPt < 1.0f && (m_pOwner->m_CurrentPathIndex + 1 < m_pOwner->m_Path.size()))
			{
				XMFLOAT3 nextPt = m_pOwner->m_Path[m_pOwner->m_CurrentPathIndex + 1];
				vLookTarget = XMLoadFloat3(&nextPt); // 看下一个点
			}
			else
			{
				vLookTarget = vCurrentPt; // 看当前点
			}
		}

		// 3. 用最终确定的 vLookTarget 重新计算方向向量
		XMVECTOR vDir = XMVectorSetY(vLookTarget - vEnemyPos, 0.0f);
		float distSq = XMVectorGetX(XMVector3LengthSq(vDir));

		// 4. 只有当看向的目标比较远时 (> 0.01) 才更新旋转
		if (distSq > 0.01f)
		{
			XMVECTOR dirNorm = XMVector3Normalize(vDir);
			float targetAngle = atan2f(XMVectorGetX(dirNorm), XMVectorGetZ(dirNorm));

			// 平滑旋转
			float currentAngle = m_pOwner->GetRotation().y;
			float diff = targetAngle - currentAngle;
			while (diff > XM_PI) diff -= XM_2PI;
			while (diff < -XM_PI) diff += XM_2PI;

			// 旋转速度：5.0f 是比较自然的速度
			float smoothAngle = currentAngle + diff * 5.0f * (float)elapsed_time;
			m_pOwner->SetRotationY(smoothAngle);
		}

		// 播放走路动画
		if (!m_pOwner->m_Animator.IsPlaying(g_pWalkAnim)) {
			if (g_pWalkAnim) m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.2f);
		}
	}
	else {
		// 停下来了
		if (!m_pOwner->m_Animator.IsPlaying(g_pIdleAnim)) {
			if (g_pIdleAnim) m_pOwner->m_Animator.PlayAnimation(g_pIdleAnim, true, 0.2f);
		}
	}

	// 贴地
	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
}
void EnemyTest::EnemyTest_StateChase::Draw() const
{

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

	// 2. 角度检测
	vToPlayer = XMVector3Normalize(vToPlayer);
	XMVECTOR vEnemyForward = XMVectorSet(sinf(m_Rotation.y), 0.0f, cosf(m_Rotation.y), 0.0f);
	float dot = XMVectorGetX(XMVector3Dot(vEnemyForward, vToPlayer));
	float halfFOV = XMConvertToRadians(m_FOVAngle * 0.5f);
	if (dot < cosf(halfFOV)) return false;

	// 3. 射线遮挡检测
	// 如果两点之间有障碍物，返回 true -> CanSeePlayer 返回 false
	if (Game_IsLineOfSightBlocked(enemyPos, playerPos)) {
		return false;
	}

	return true;
}

EnemyTest::EnemyTest(const DirectX::XMFLOAT3& position)
	: m_position(position) 
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
			// 和玩家一样支持 crossfade 的版本：
			m_Animator.PlayAnimation(g_pIdleAnim, true);   // 第一次播放，crossfade 可以不写或 0
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
	

		// 2. 如果有其它动作，也可以在这里加载：
		// g_pSkinningModel->LoadAnimation("Run",   "resource/Model/Zombie/Zombie Run.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Walk",  "resource/Model/Zombie/Zombie Walk.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Attack","resource/Model/Zombie/Zombie Attack.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Scream","resource/Model/Zombie/Zombie Scream.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Dying","resource/Model/Zombie/Zombie Dying.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Reaction Hit","resource/Model/Zombie/Zombie Reaction Hit.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Scratch Idle","resource/Model/Zombie/Zombie Scratch Idle.fbx", 1.0f);



		// 3. 获取动画指针
		if (g_pIdleAnim == nullptr) g_pIdleAnim = g_pSkinningModel->GetDefaultAnimation();
		if (g_pWalkAnim == nullptr)   g_pWalkAnim = g_pSkinningModel->GetAnimation("Walk");
		if (g_pAttackAnim == nullptr) g_pAttackAnim = g_pSkinningModel->GetAnimation("Attack");
		if (g_pScreamAnim == nullptr) g_pScreamAnim = g_pSkinningModel->GetAnimation("Scream");
		if (g_pDyingAnim == nullptr)  g_pDyingAnim = g_pSkinningModel->GetAnimation("Dying");
		if (g_pReaction_HitAnim == nullptr) g_pReaction_HitAnim = g_pSkinningModel->GetAnimation("Reaction Hit");
		if (g_pScratchIdleAnim == nullptr) g_pScratchIdleAnim = g_pSkinningModel->GetAnimation("Scratch Idle");
	}
}

void EnemyTest::UnloadAssets()
{
	if (g_pSkinningModel != nullptr) {
		g_pSkinningModel->Release();
		delete g_pSkinningModel;
		g_pSkinningModel = nullptr;
		g_pIdleAnim = nullptr;
		g_pWalkAnim = nullptr;
		g_pAttackAnim = nullptr;
		g_pScreamAnim = nullptr;
		g_pDyingAnim = nullptr;
		g_pReaction_HitAnim = nullptr;
	}
}

void EnemyTest::SetPosition(const DirectX::XMFLOAT3& pos)
{
	if (m_bIsDead) return;
	m_position = pos;
}

void EnemyTest::Damage(float damage, bool isMelee)
{
	if (m_bIsDead) return;

	// ============================================================
	// 背刺判定 (Backstab)
	// ============================================================
	// 只有近战才能触发背刺
	if (isMelee)
	{
		// 1. 计算 [敌人 -> 玩家] 的方向向量
		XMFLOAT3 playerPos = Player_GetPosition();
		XMVECTOR vToPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&m_position);
		vToPlayer = XMVectorSetY(vToPlayer, 0.0f); // 忽略高度差
		vToPlayer = XMVector3Normalize(vToPlayer);

		// 2. 获取敌人的正前方向量
		XMVECTOR vForward = XMVectorSet(sinf(m_Rotation.y), 0.0f, cosf(m_Rotation.y), 0.0f);
		float dot = XMVectorGetX(XMVector3Dot(vForward, vToPlayer));

		if (dot < -0.2f)
		{
			// >>> 触发背刺：一击必杀 <<<
			Score_AddScore(100);

			m_HP = 0.0f;
			m_bIsDead = true;
			m_DeathTimer = 3.5f; // 尸体存在时间

			// 直接播放死亡动画
			if (g_pDyingAnim) {
				m_Animator.PlayAnimation(g_pDyingAnim, false, 0.1f);
			}

			// 掉落逻辑 (30% 几率掉子弹)
			int rate = rand() % 100;
			if (rate < 30) {
				DropItem_Spawn(m_position, 4);
			}

			// 【关键】直接返回，不再执行后续的“警觉”或“普通受击”逻辑
			return;
		}
	}

	// ============================================================
	// 下面是原有的普通受击逻辑
	// ============================================================

	// 1. 如果之前没发现玩家，现在发现了 (且没被背刺死)
	if (!m_bAlertedStatus) {
		m_bAlertedStatus = true;
		ChangeState(new EnemyTest_StateChase(this));

		if (g_pScreamAnim) {
			m_Animator.PlayAnimation(g_pScreamAnim, false, 0.1f);
		}

		m_HP -= damage;
		return;
	}

	// 2. 正常战斗中的扣血
	m_HP -= damage;

	if (m_HP <= 0.0f) {
		// --- 普通死亡 ---
		Score_AddScore(100);
		m_HP = 0.0f;
		m_bIsDead = true;
		m_Animator.PlayAnimation(g_pDyingAnim, false, 0.1f);
		m_DeathTimer = 3.5f;

		int rate = rand() % 100;
		if (rate < 15)
		{
			DropItem_Spawn(m_position, 4);
		}
	}
	else {
		// --- 存活时的受击反馈 ---
		if (isMelee) {
			// 近战攻击：播放硬直动画
			m_Animator.PlayAnimation(g_pReaction_HitAnim, false, 0.1f);
		}
	}
}

AABB EnemyTest::GetAABB()
{
	float hw = 0.5f; // 半宽
	float h = 2.0f;  // 高度
	return {
		{ m_position.x - hw, m_position.y,     m_position.z - hw },
		{ m_position.x + hw, m_position.y + h, m_position.z + hw }
	};
}

void EnemyTest::ChangeState(State* pNextState)
{
	Enemy::ChangeState(pNextState);
}
void EnemyTest::ApplyKnockback(const DirectX::XMVECTOR& direction, float force)
{
	// 如果已经死亡，不处理物理效果
	if (m_bIsDead) return;

	// 1. 计算击退向量 (忽略 Y 轴，防止被打飞上天)
	XMVECTOR knockDir = XMVectorSetY(direction, 0.0f);
	knockDir = XMVector3Normalize(knockDir);

	// 2. 获取当前位置并施加位移
	XMVECTOR currentPos = XMLoadFloat3(&m_position);
	XMVECTOR newPos = currentPos + knockDir * force;

	// 3. 更新位置
	XMStoreFloat3(&m_position, newPos);

	// 4. 确保贴地 (非常重要，否则击退后可能悬空或穿地)
	m_position.y = MeshField_GetHeight(m_position.x, m_position.z);
}
void EnemyTest::SetAlerted(bool alerted) { m_bAlertedStatus = alerted; }


bool EnemyTest::IsDestroyed() const
{
	return m_bIsDead && (m_DeathTimer <= 0.0f);
}

void EnemyTest::Update(double elapsed_time)
{
	if (m_bIsDead) {
		m_DeathTimer -= (float)elapsed_time;
		m_Animator.Update(elapsed_time);
		return;
	}

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

	// 2. 准备渲染环境 (绑定 Shader, InputLayout 等)
	SkinningShader_3D_Begin();

	// 3. 设置世界矩阵、视图矩阵、投影矩阵
	SkinningShader_3D_SetWorldMatrix(World);
	SkinningShader_3D_SetViewMatrix(view);
	SkinningShader_3D_SetProjectMatrix(proj);

	// 4. 获取并设置骨骼变换矩阵
	std::vector<XMMATRIX> bones = const_cast<Animator&>(m_Animator).GetFinalBoneMatrices(g_pSkinningModel->GetSkeleton());

	SkinningShader_3D_SetBoneTransforms(bones);

	// 5. 调用模型自身的 Draw 提交渲染命令
	const_cast<SkinningModel*>(g_pSkinningModel)->Draw();

}

EnemyTest::EnemyTest_StateSearch::EnemyTest_StateSearch(EnemyTest* pOwner)
	: m_pOwner(pOwner), m_SearchTimer(2.5f) // 动作持续 2.5 秒
{
	// 播放挠头/寻找动作
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

	// 2. 计时器倒数
	m_SearchTimer -= (float)elapsed_time;

	if (m_SearchTimer <= 0.0f)
	{
		m_pOwner->SetAlerted(false); // 解除警觉
		m_pOwner->ChangeState(new EnemyTest_StatePatrol(m_pOwner));
		return;
	}

	// ... 维持高度等 ...
	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
}