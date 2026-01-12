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
	: m_pOwner(pOwner), m_RePathTimer(0.0f)
{
	// 切换到追逐状态时，播放移动动画
	if (g_pWalkAnim) {
		m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.3f); // 0.3秒平滑过渡
	}
}
EnemyTest::EnemyTest_StateChase::EnemyTest_StateChase(EnemyTest* pOwner, const DirectX::XMFLOAT3& targetPos)
	: m_pOwner(pOwner), m_RePathTimer(0.0f)
{
	// 1. 播放移动动画
	if (g_pWalkAnim) {
		m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.3f);
	}

	// 2. 【核心逻辑】欺骗状态机，让它认为“刚刚丢失了玩家视野”
	m_pOwner->m_HasLostSight = true;

	// 3. 将声音位置设定为搜索目标
	m_pOwner->m_PersonalSearchTarget = targetPos;

	// 4. 更新最后已知位置（防止逻辑出错）
	m_pOwner->m_LastKnownPosition = targetPos;
}
void EnemyTest::EnemyTest_StateChase::Update(double elapsed_time)
{
	// =================================================================================
	// 0. 预处理：更新受阻计时器
	// =================================================================================
	if (m_pOwner->m_StuckTimer > 0.0f) {
		m_pOwner->m_StuckTimer -= (float)elapsed_time;
	}

	// =================================================================================
	// 1. 基础信息
	// =================================================================================
	XMFLOAT3 playerPos = Player_GetPosition();
	XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position);

	// 处理尖叫动画 (保持不变)
	if (m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
		if (m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.9f) {
			m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
			return;
		}
	}

	// =================================================================================
	// 2. 视野判定 & 目标锁定
	// =================================================================================
	bool canSee = m_pOwner->CanSeePlayer();
	XMFLOAT3 targetPos;

	if (canSee)
	{
		// --- 情况 A: 看得见 ---
		m_pOwner->m_LastKnownPosition = playerPos;
		m_pOwner->m_HasLostSight = false;
		m_pOwner->m_PersonalSearchTarget = playerPos; // 实时更新目标
		targetPos = playerPos;
	}
	else
	{
		// --- 情况 B: 看不见 (视线被挡住) ---

		// 刚丢失视野的那一帧，计算一个分散的搜索点
		if (!m_pOwner->m_HasLostSight)
		{
			XMFLOAT3 basePos = m_pOwner->m_LastKnownPosition;

			// 尝试生成一个不在墙里的搜索点
			bool foundValidSpot = false;
			for (int i = 0; i < 5; ++i) // 尝试5次
			{
				float randomAngle = (rand() % 360) * XM_2PI / 360.0f;
				float randomDist = 1.0f + (rand() % 100) / 100.0f;

				XMFLOAT3 testPos;
				testPos.x = basePos.x + sinf(randomAngle) * randomDist;
				testPos.z = basePos.z + cosf(randomAngle) * randomDist;
				testPos.y = basePos.y;

				// 构造一个小盒子检测这个点是不是墙
				AABB testBox = {
					{testPos.x - 0.2f, testPos.y, testPos.z - 0.2f},
					{testPos.x + 0.2f, testPos.y + 1.0f, testPos.z + 0.2f}
				};

				if (!Game_CheckCollisionWithWalls(testBox)) {
					m_pOwner->m_PersonalSearchTarget = testPos;
					foundValidSpot = true;
					break;
				}
			}

			// 如果随机的点都在墙里，就老老实实去最后看到的那个确切位置
			if (!foundValidSpot) {
				m_pOwner->m_PersonalSearchTarget = basePos;
			}
		}

		m_pOwner->m_HasLostSight = true;

		// 【关键】：看不见的时候，目标是那个分散的搜索点，不是玩家实时位置！
		targetPos = m_pOwner->m_PersonalSearchTarget;
	}

	// =================================================================================
	// 3. 检查是否“到达”了搜索点
	// =================================================================================
	if (m_pOwner->m_HasLostSight)
	{
		XMVECTOR vTarget = XMLoadFloat3(&targetPos);
		float distToTarget = XMVectorGetX(XMVector3Length(XMVectorSetY(vTarget - vEnemyPos, 0.0f)));

		//定义“到达”的条件
		bool hasArrived = false;

		// 条件 A: 距离足够近
		if (distToTarget < 0.6f) {
			hasArrived = true;
		}

		// 条件 B: A* 路径已经走完了 (且确实是在用 A* 模式)
		// 防止因为物理阻挡导致离终点还有点距离，但路已经走完了的情况
		if (!m_pOwner->m_Path.empty() && m_pOwner->m_CurrentPathIndex >= m_pOwner->m_Path.size()) {
			hasArrived = true;
		}

		// 如果满足任意到达条件
		if (hasArrived)
		{
			m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateSearch(m_pOwner));
			return;
		}
	}

	// =================================================================================
	// 4. 攻击逻辑 (只在看得见时触发)
	// =================================================================================
	if (!m_pOwner->m_HasLostSight)
	{
		XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);
		float distToPlayer = XMVectorGetX(XMVector3Length(XMVectorSetY(vPlayerPos - vEnemyPos, 0.0f)));

		bool inAttackRange = (distToPlayer < m_pOwner->m_AttackRadius);
		bool isPlayingAttack = m_pOwner->m_Animator.IsPlaying(g_pAttackAnim);

		if (inAttackRange || (isPlayingAttack && m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.95f))
		{
			// 1. 播放攻击动画
			m_pOwner->m_Animator.PlayAnimation(g_pAttackAnim, true, 0.1f);
			m_pOwner->m_Animator.SetSpeedScale(1.2f); // 稍微加快一点攻击速度

			// =============================================================
			// 2. 【核心修复】伤害判定逻辑
			// =============================================================
			float progress = m_pOwner->m_Animator.GetCurrentAnimationProgress();

			// A. 如果动画刚开始（比如循环播放重置了），重置伤害标记
			if (progress < 0.2f) {
				m_HasDealtDamageInThisCycle = false;
			}

			// B. 如果动画播放到了 30% (假设这是挥手击中的时刻)，且这一轮还没扣过血
			if (progress > 0.3f && !m_HasDealtDamageInThisCycle)
			{
				// 只有距离足够近才能扣血（防止玩家已经跑远了还受到伤害）
				// 重新计算一下距离
				float currentDist = XMVectorGetX(XMVector3Length(vPlayerPos - vEnemyPos));
				if (currentDist < m_pOwner->m_AttackRadius + 0.5f) //稍微给点宽容度
				{
					// 调用全局函数扣玩家血量
					Player_Damage(10.0f);

					// 可以在这里播放攻击音效
					// PlaySound("Punch");
				}

				// 标记为已攻击，防止这一刀重复扣血
				m_HasDealtDamageInThisCycle = true;
			}
			// =============================================================

			// 3. 攻击时始终朝向玩家
			XMVECTOR dir = XMVector3Normalize(XMVectorSetY(vPlayerPos - vEnemyPos, 0.0f));
			float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
			m_pOwner->SetRotationY(angle);

			return; // 攻击中不移动
		}
	}

	// =================================================================================
	// 5. 移动决策 (强制 A* 绕路)
	// =================================================================================
	m_pOwner->m_Animator.SetSpeedScale(1.0f);

	XMVECTOR vTarget = XMLoadFloat3(&targetPos);
	XMVECTOR vPos = XMLoadFloat3(&m_pOwner->m_position);
	float distToTarget = XMVectorGetX(XMVector3Length(XMVectorSetY(vTarget - vPos, 0.0f)));

	// --- 智能程度决策 ---
	// useAStar = true  -> 聪明模式 (计算绕路)
	// useAStar = false -> 笨蛋模式 (直线冲锋)

	bool useAStar = false;

	// 优先级 1: 撞墙了 (最高优先级)
	// 如果之前撞墙了 (StuckTimer > 0)，说明刚才的尝试失败了，必须立刻动脑子绕路
	if (m_pOwner->m_StuckTimer > 0.0f)
	{
		useAStar = true;
	}
	// 优先级 2: 丢失视野 (HasLostSight)
	else if (m_pOwner->m_HasLostSight)
	{
		// 如果距离还很远 (> 6.0米)，不要立刻用 A*。
		if (distToTarget > 6.0f)
		{
			useAStar = false; // 远距离 -> 走直线 (哪怕前面有墙)
		}
		else
		{
			useAStar = true;  // 近距离 -> 启动 A* 找路
		}
	}
	// 优先级 3: 看得见玩家 (正常追击)
	else
	{
		// 如果看得见，只在直线被特殊地形(如栏杆)挡住时才用 A*
		bool isLineBlocked = Pathfinder::RaycastHit(m_pOwner->m_position, targetPos);
		useAStar = isLineBlocked;
	}


	// --- 执行决策 ---
	XMVECTOR vSeekDir = XMVectorSet(0, 0, 0, 0);
	float distToNextNode = 0.0f; // 用于后续的减速判断

	if (!useAStar)
	{
		// --- 方案 A: 直线追击 (笨蛋模式 / 远距离模式) ---
		// 清空路径，直接冲向目标
		m_pOwner->m_Path.clear();
		m_pOwner->m_CurrentPathIndex = 0;

		XMVECTOR toTarget = vTarget - vEnemyPos;
		toTarget = XMVectorSetY(toTarget, 0.0f);

		// 只要没重合就一直走
		if (XMVectorGetX(XMVector3LengthSq(toTarget)) > 0.001f) {
			vSeekDir = XMVector3Normalize(toTarget);
		}

		distToNextNode = distToTarget; // 直线模式下，下一站就是终点
	}
	else
	{
		// --- 方案 B: A* 寻路 (聪明模式) ---
		m_RePathTimer -= (float)elapsed_time;

		if (m_pOwner->m_Path.empty() || m_RePathTimer <= 0.0f) {
			m_pOwner->m_Path = Pathfinder::FindPath(m_pOwner->m_position, targetPos);
			m_pOwner->m_CurrentPathIndex = 0;
			// 撞墙时刷新快一点
			m_RePathTimer = (m_pOwner->m_StuckTimer > 0.0f) ? 0.3f : 0.8f;
			if (m_pOwner->m_Path.empty() && m_pOwner->m_HasLostSight)
			{
				// 计算物理距离
				float dist = XMVectorGetX(XMVector3Length(XMVectorSetY(vTarget - vPos, 0.0f)));

				// 只有当距离真的很近（< 0.5f）或者 真的很远但算不出路时，才放弃
				if (dist < 0.5f) {
					// 确实到了，切 Search
					m_pOwner->ChangeState(new EnemyTest::EnemyTest_StateSearch(m_pOwner));
					return;
				}
				else {
					useAStar = false;
					XMVECTOR toTarget = vTarget - vEnemyPos;
					toTarget = XMVectorSetY(toTarget, 0.0f);
					if (XMVectorGetX(XMVector3LengthSq(toTarget)) > 0.001f) {
						vSeekDir = XMVector3Normalize(toTarget);
					}
				}
			}
		}

		if (!m_pOwner->m_Path.empty() && m_pOwner->m_CurrentPathIndex < m_pOwner->m_Path.size()) {
			XMFLOAT3 nextNode = m_pOwner->m_Path[m_pOwner->m_CurrentPathIndex];
			XMVECTOR vNext = XMLoadFloat3(&nextNode);
			XMVECTOR toNext = vNext - vEnemyPos;
			toNext = XMVectorSetY(toNext, 0.0f);
			distToNextNode = XMVectorGetX(XMVector3Length(toNext));

			if (distToNextNode < 0.8f) { // 路点阈值
				m_pOwner->m_CurrentPathIndex++;
			}
			else {
				vSeekDir = XMVector3Normalize(toNext);
			}
		}
	}

	// =================================================================================
	// 6. 群聚排斥 (防止重叠)
	// =================================================================================
	XMVECTOR vSeparation = XMVectorSet(0, 0, 0, 0);
	int neighborCount = 0;
	float separateRadius = 1.0f;
	for (EnemyTest* other : EnemyTest::g_AllEnemies) {
		if (other == m_pOwner || other->IsDestroyed()) continue;
		XMVECTOR vToOther = XMLoadFloat3(&other->GetPosition()) - vEnemyPos;
		float dSq = XMVectorGetX(XMVector3LengthSq(vToOther));
		if (dSq < separateRadius * separateRadius && dSq > 0.0001f) {
			float strength = 1.0f - (sqrtf(dSq) / separateRadius);
			vSeparation += XMVector3Normalize(vEnemyPos - XMLoadFloat3(&other->GetPosition())) * strength;
			neighborCount++;
		}
	}
	if (neighborCount > 0) vSeparation /= (float)neighborCount;

	// =================================================================================
	// 7. 执行移动
	// =================================================================================
	// 如果快到了搜索点，减小寻路力度，防止抖动
	float seekWeight = 1.0f;
	XMVECTOR vFinalDir = vSeekDir * seekWeight + vSeparation * 2.5f;

	if (XMVectorGetX(XMVector3LengthSq(vFinalDir)) > 0.01f)
	{
		vFinalDir = XMVector3Normalize(vFinalDir);
		float moveSpeed = 1.0f; // 你可以把这个变成成员变量

		// 计算这一帧的【期望】位移量
		XMVECTOR vDelta = vFinalDir * moveSpeed * (float)elapsed_time;
		XMFLOAT3 delta;
		XMStoreFloat3(&delta, vDelta);

		// 备份当前位置 (用于回退)
		XMFLOAT3 originalPos = m_pOwner->m_position;
		// 备份起始位置 (用于计算实际移动了多少)
		XMFLOAT3 startPosOfFrame = m_pOwner->m_position;

		// --- 步骤 A: 尝试移动 X 轴 ---
		m_pOwner->m_position.x += delta.x;
		if (Game_CheckCollisionWithWalls(m_pOwner->GetAABB()))
		{
			// 撞墙了！仅回退，【不】立刻标记为 Stuck
			m_pOwner->m_position.x = originalPos.x;
		}

		// --- 步骤 B: 尝试移动 Z 轴 ---
		m_pOwner->m_position.z += delta.z;
		if (Game_CheckCollisionWithWalls(m_pOwner->GetAABB()))
		{
			// 撞墙了！仅回退，【不】立刻标记为 Stuck
			m_pOwner->m_position.z = originalPos.z;
		}

		// --- 步骤 C: 智能卡死检测 (Smart Stuck Detection) ---

		// 1. 计算这一帧【实际】移动了多远
		float actualDx = m_pOwner->m_position.x - startPosOfFrame.x;
		float actualDz = m_pOwner->m_position.z - startPosOfFrame.z;
		float actualDistSq = actualDx * actualDx + actualDz * actualDz;

		// 2. 计算【期望】移动距离的平方 (稍微打个折，比如 10%)
		// 如果实际移动距离 < 期望距离的 10%，说明被死死卡住了（角落）
		float expectedDistSq = (delta.x * delta.x + delta.z * delta.z);

		// 如果应该移动但几乎没动 (注意：要排除本来就没想动的情况)
		if (expectedDistSq > 0.00001f && actualDistSq < expectedDistSq * 0.1f)
		{
			// >>> 确实卡死了 (走进死胡同/角落) <<<
			m_pOwner->m_StuckTimer = 1.0f;

			// 只有真的卡死了，才去强制推进路径索引或触发重寻路
			if (useAStar) {
				m_pOwner->m_CurrentPathIndex++;
			}
		}
		else
		{
			// >>> 只是在贴墙滑动 <<<
			// 此时我们认为移动是成功的，不需要恐慌。
			// 这会让敌人在墙角转弯时更丝滑，而不会鬼畜。
		}

		// --- 步骤 D: 紧急逃逸 (保持不变) ---
		if (Game_CheckCollisionWithWalls(m_pOwner->GetAABB()))
		{
			m_pOwner->m_position = originalPos;
		}

		// 旋转 
		float targetAngle = atan2f(XMVectorGetX(vFinalDir), XMVectorGetZ(vFinalDir));
		float currentAngle = m_pOwner->GetRotation().y;

		// 计算角度差 (处理 -PI 到 PI 的突变问题，防止绕大圈)
		float diff = targetAngle - currentAngle;
		while (diff > XM_PI) diff -= XM_2PI;
		while (diff < -XM_PI) diff += XM_2PI;

		// 插值系数：10.0f * dt 表示每秒修正 10 倍差距，值越小越滑，值越大越硬
		// 如果还是觉得抖，可以把 10.0f 改成 5.0f
		float smoothAngle = currentAngle + diff * 10.0f * (float)elapsed_time;

		m_pOwner->SetRotationY(smoothAngle);

		// 播放动画
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

	// 3. 【新增】射线遮挡检测
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
	// 【新增】背刺判定 (Backstab)
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

		// 3. 计算点积 (Dot Product)
		// Dot > 0 : 玩家在敌人前方
		// Dot < 0 : 玩家在敌人后方
		// 这里设阈值为 -0.2f (大约背后 100 度范围)，判定比较宽容，方便玩家操作
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

		// 切换到追逐状态
		ChangeState(new EnemyTest_StateChase(this));

		// 播放尖叫/发现动画
		if (g_pScreamAnim) {
			m_Animator.PlayAnimation(g_pScreamAnim, false, 0.1f);
		}

		m_HP -= damage;
		// 注意：原来的逻辑里，未警觉状态下被打第一下是不会死的（除非你在这里也加死亡判定）
		// 但因为现在有了背刺逻辑，这一刀通常是正面硬刚，不致死也合理
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
	// 注意：View 和 Projection 通常由 Camera 类提供
	SkinningShader_3D_SetWorldMatrix(World);
	SkinningShader_3D_SetViewMatrix(view);
	SkinningShader_3D_SetProjectMatrix(proj);

	// 4. 获取并设置骨骼变换矩阵
	// 从 Animator 获取当前帧经过 invBindPose 处理后的最终矩阵数组
	std::vector<XMMATRIX> bones = const_cast<Animator&>(m_Animator).GetFinalBoneMatrices(g_pSkinningModel->GetSkeleton());

	// 调用你提供的 Shader 接口，利用 Map/Unmap 更新到 GPU 的 b3 缓冲区
	SkinningShader_3D_SetBoneTransforms(bones);

	// 5. 调用模型自身的 Draw 提交渲染命令
	// 这里的 Draw 会负责绑定顶点/索引缓冲区并执行 DrawIndexed
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