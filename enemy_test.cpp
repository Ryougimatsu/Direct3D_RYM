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
const Animation* EnemyTest::g_pHitMeleeAnim = nullptr;
const Animation* EnemyTest::g_pHitBulletAnim = nullptr;
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

		// 3. 碰撞检测
		if (Game_CheckCollisionWithWalls(m_pOwner->GetAABB()))
		{
			// 撞墙了！
			// 回退到撞墙前的位置
			m_pOwner->m_position = oldPos;

			// 此路不通，立刻重新选一个随机目标点 (这样敌人就会转身走开)
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
	bool isHitMelee = (g_pHitMeleeAnim && m_pOwner->m_Animator.IsPlaying(g_pHitMeleeAnim));
	bool isHitBullet = (g_pHitBulletAnim && m_pOwner->m_Animator.IsPlaying(g_pHitBulletAnim));
	if (isHitMelee || isHitBullet)
	{
		float progress = m_pOwner->m_Animator.GetCurrentAnimationProgress();

		if (progress < 0.9f)
		{
			m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);


			return;
		}
	}
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

			// 增加尝试次数，确保能找到合适的位置
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

	XMVECTOR vKnockVel = XMLoadFloat3(&m_pOwner->m_KnockbackVelocity);
	// 如果击退速度大于 0.1，说明还在滑动，禁止 AI 寻路移动
	if (XMVectorGetX(XMVector3LengthSq(vKnockVel)) > 0.01f)
	{
		// 依然保持贴地，防止飞天
		m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);

		// 更新旋转朝向玩家（可选，如果你想让他在滑行时盯着玩家）
		XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);
		XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position);
		if (distToPlayer > 0.1f) {
			XMVECTOR dir = XMVector3Normalize(XMVectorSetY(vPlayerPos - vEnemyPos, 0.0f));
			float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
			m_pOwner->SetRotationY(angle);
		}

		return; // 直接返回，不执行下面的 MoveToTarget
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
		g_pSkinningModel->LoadAnimation("Walk",  "resource/Model/Zombie/Zombie Walk.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Attack","resource/Model/Zombie/Zombie Attack.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Scream","resource/Model/Zombie/Zombie Scream.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Dying","resource/Model/Zombie/Zombie Dying.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Reaction Hit","resource/Model/Zombie/Zombie Reaction Hit.fbx", 1.0f);
		g_pSkinningModel->LoadAnimation("Scratch Idle","resource/Model/Zombie/Zombie Scratch Idle.fbx", 1.0f);
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

	m_bAlertedStatus = true; // 依然标记为警觉
	ChangeState(new EnemyTest_StateHit(this));
	m_Path.clear();
	m_CurrentPathIndex = 0;
	const Animation* animToPlay = nullptr;
	float targetSpeed = 1.0f;
	bool shouldPlay = false;
	float knockbackForce = 0.3f; // 默认子弹力度

	if (isMelee)
	{
		// 近战逻辑
		shouldPlay = true;
		animToPlay = g_pHitMeleeAnim;
		targetSpeed = 0.8f;      // 慢速体现沉重
		m_HitAnimCooldown = 1.0f;
		knockbackForce = 4.0f;   // 【建议】近战力度给大点，不然看不出滑行
	}
	else
	{
		// 子弹逻辑
		if (m_HitAnimCooldown <= 0.0f) {
			if (damage > 10.0f || (rand() % 100 < 40)) {
				shouldPlay = true;
			}
		}
		if (shouldPlay) {
			animToPlay = g_pHitBulletAnim;
			targetSpeed = 1.0f;
			m_HitAnimCooldown = 0.5f;
		}
		knockbackForce = 0.5f; // 子弹力度小
	}

	// ==========================================================
	// 4. 【最后一步】播放受击动画 & 物理击退
	// ==========================================================
	// 放在最后执行，确保它覆盖掉 ChangeState 里的走路动画
	if (shouldPlay && animToPlay) {
		float blendTime = isMelee ? 0.02f : 0.05f;
		m_Animator.PlayAnimation(animToPlay, false, blendTime);
		m_Animator.SetSpeedScale(targetSpeed);
	}

	// 执行物理击退
	XMFLOAT3 pPos = Player_GetPosition();
	XMVECTOR vToEnemy = XMLoadFloat3(&m_position) - XMLoadFloat3(&pPos);
	ApplyKnockback(vToEnemy, knockbackForce);

	// ==========================================================
	// 5. 扣血与死亡
	// ==========================================================
	m_HP -= damage;

	if (m_HP <= 0.0f) {
		Score_AddScore(100);
		m_HP = 0.0f;
		m_bIsDead = true;
		m_Animator.PlayAnimation(g_pDyingAnim, false, 0.2f);
		m_DeathTimer = 3.5f;
		m_KnockbackVelocity = { 0,0,0 }; // 死亡消除滑行

		int rate = rand() % 100;
		if (rate < 15) {
			DropItem_Spawn(m_position, 4);
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
	if (m_bIsDead) return;

	XMVECTOR knockDir = XMVectorSetY(direction, 0.0f);
	knockDir = XMVector3Normalize(knockDir);

	float speedMultiplier = 3.0f;

	// 直接设置速度，不要延迟
	XMStoreFloat3(&m_KnockbackVelocity, knockDir * force * speedMultiplier);

	// 将延迟设为 0，让物理在下一帧立刻生效
	m_KnockbackDelayTimer = 0.0f;
}
void EnemyTest::SetAlerted(bool alerted) { m_bAlertedStatus = alerted; }


bool EnemyTest::IsDestroyed() const
{
	return m_bIsDead && (m_DeathTimer <= 0.0f);
}

void EnemyTest::Update(double elapsed_time)
{
	float dt = (float)elapsed_time;
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
	if (m_HitAnimCooldown > 0.0f) {
		m_HitAnimCooldown -= (float)elapsed_time;
	}

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

// ========================================================
// 状态机 - 受击 (新增)
// ========================================================
EnemyTest::EnemyTest_StateHit::EnemyTest_StateHit(EnemyTest* pOwner)
	: m_pOwner(pOwner), m_StunTimer(0.0f)
{
	m_StunTimer = m_pOwner->m_HitAnimCooldown;
	if (m_StunTimer <= 0.0f) m_StunTimer = 0.5f;
}

void EnemyTest::EnemyTest_StateHit::Update(double elapsed_time)
{
	m_StunTimer -= (float)elapsed_time;

	// 1. 物理贴地 (受击时也要贴地)
	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);

	// 2. 检查退出条件：
	// 条件 A: 硬直时间结束
	// 条件 B: 且 击退速度已经很小了 (防止还在滑行时就开始走路)
	XMVECTOR vKnock = XMLoadFloat3(&m_pOwner->m_KnockbackVelocity);
	float currentSpeed = XMVectorGetX(XMVector3LengthSq(vKnock));

	if (m_StunTimer <= 0.0f && currentSpeed < 0.01f)
	{
		// 受击结束，切换回追逐状态
		// 只有等到完全停稳了，才把控制权交给 AI，这样就不会被导航网格拽回去了
		m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateChase(m_pOwner));
		return;
	}
}