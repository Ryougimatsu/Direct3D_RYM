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
using namespace DirectX;


const Animation* EnemyTest::g_pIdleAnim = nullptr;
const Animation* EnemyTest::g_pWalkAnim = nullptr;
const Animation* EnemyTest::g_pAttackAnim = nullptr;
const Animation* EnemyTest::g_pScreamAnim = nullptr;
const Animation* EnemyTest::g_pDyingAnim = nullptr;
const Animation* EnemyTest::g_pReaction_HitAnim = nullptr;
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
	// 在中心点周围的范围内随机生成 X 和 Z 坐标
	m_TargetPoint.x = m_PatrolOrigin.x + GetRandomFloat(-MAX_WANDER_RADIUS, MAX_WANDER_RADIUS);
	m_TargetPoint.z = m_PatrolOrigin.z + GetRandomFloat(-MAX_WANDER_RADIUS, MAX_WANDER_RADIUS);

	// 立即通过 MeshField 获取该点在地面上的正确高度
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
		XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position); // 需要重新获取一下位置
		XMVECTOR vSoundPos = XMLoadFloat3(&soundPos);
		XMVECTOR toSound = vSoundPos - vEnemyPos;
		float distSq = XMVectorGetX(XMVector3LengthSq(toSound));

		if (distSq < (soundRadius * soundRadius)) {
			m_TargetPoint = soundPos;
			m_WaitTimer = 0.0f;

			if (!m_bAlerted && !m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
				m_pOwner->m_Animator.PlayAnimation(g_pScreamAnim, false, 0.2f);
				m_bAlerted = true;
				m_pOwner->SetAlerted(true);
			}
		}
	}

	if (m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
		// 如果尖叫进度没到 90%，直接返回，不执行下面的位移代码
		if (m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.9f) {
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
		XMVECTOR vNewPos = vPos + dir * patrolSpeed * (float)elapsed_time;
		XMStoreFloat3(&m_pOwner->m_position, vNewPos);

		float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
		m_pOwner->SetRotationY(angle);
		m_pOwner->m_Animator.SetSpeedScale(patrolSpeed / 1.0f);
	}
	else
	{
		// 到达随机目标点，进入等待，并生成下一个目标点
		m_WaitTimer = WAIT_DURATION;
		PickRandomTarget(); // 关键：下次出发前已经选好了新位置
		m_bAlerted = false;// 重置警觉状态
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
	if (g_pWalkAnim) {
		m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.3f); // 0.3秒平滑过渡
	}
}


void EnemyTest::EnemyTest_StateChase::Update(double elapsed_time)
{
	XMFLOAT3 playerPos = Player_GetPosition();
	XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);
	XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position);

	XMVECTOR toPlayer = XMVectorSubtract(vPlayerPos, vEnemyPos);
	toPlayer = XMVectorSetY(toPlayer, 0.0f); // 忽略高度差进行导航

	float dist = XMVectorGetX(XMVector3Length(toPlayer));

	if (m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
		// 如果播放进度小于 90%，就跳过移动逻辑
		if (m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.9f) {

			// 记得更新重力/高度，防止穿帮
			m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);

			// 仍然需要更新动画机的时间
			// (注意：Enemy::Update 里已经调用了 m_Animator.Update，所以这里直接 return 即可)
			return;
		}
	}

	// --- 1. 状态判断：攻击范围判定 ---
	bool inAttackRange = (dist < m_pOwner->m_AttackRadius);

	bool isPlayingAttack = m_pOwner->m_Animator.IsPlaying(g_pAttackAnim);
	float progress = m_pOwner->m_Animator.GetCurrentAnimationProgress();
	if (inAttackRange || (m_pOwner->m_Animator.IsPlaying(g_pAttackAnim) && m_pOwner->m_Animator.GetCurrentAnimationProgress() < 0.95f))
	{
		// 如果是从移动切换到攻击，或者是继续攻击
		m_pOwner->m_Animator.PlayAnimation(g_pAttackAnim, true, 0.2f);
		m_pOwner->m_Animator.SetSpeedScale(1.2f); // 稍微加快一点动作

		// 2. 动画切换后再获取进度
		float progress = m_pOwner->m_Animator.GetCurrentAnimationProgress();

		// --- 核心修复：更严格的伤害判定 ---
		const float HIT_POINT = 0.4f; // 击中点提前到 40%

		// 如果进度非常小，说明是新的一轮或刚切换，重置标记
		if (progress < 0.1f) {
			m_HasDealtDamageInThisCycle = false;
		}

		// 判定扣血：必须是攻击动画，且进度越过击中点，且本轮没扣过
		if (m_pOwner->m_Animator.IsPlaying(g_pAttackAnim) &&
			progress >= HIT_POINT &&
			!m_HasDealtDamageInThisCycle)
		{
			if (inAttackRange) {
				Player_Damage(10.0f);
			}
			m_HasDealtDamageInThisCycle = true; // 锁定本轮伤害
		}
		XMVECTOR dir = XMVector3Normalize(toPlayer);
		float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
		m_pOwner->SetRotationY(angle);

		return;
	}

	m_pOwner->m_Animator.SetSpeedScale(1.0f);

	bool isBlocked = Pathfinder::RaycastHit(m_pOwner->m_position, playerPos);

	if (!isBlocked)
	{
		// -----------------------------------------------------
		// 【情况 A：直线通畅】-> 直接追！
		// -----------------------------------------------------

		// 清空路径，表示现在不需要 A*
		m_pOwner->m_Path.clear();
		m_pOwner->m_CurrentPathIndex = 0;

		// 直接计算朝向玩家的向量
		XMVECTOR vPos = XMLoadFloat3(&m_pOwner->m_position);
		XMVECTOR vTarget = XMLoadFloat3(&playerPos);
		XMVECTOR toPlayer = vTarget - vPos;
		toPlayer = XMVectorSetY(toPlayer, 0.0f); // 忽略高度

		// --- 简单的直线移动 ---
		XMVECTOR dir = XMVector3Normalize(toPlayer);

		XMVECTOR separationForce = XMVectorSet(0, 0, 0, 0);
		float separateRadius = 1.2f; // 排斥半径

		for (EnemyTest* other : EnemyTest::g_AllEnemies) {
			if (other == m_pOwner || other->IsDestroyed()) continue;

			XMVECTOR vOtherPos = XMLoadFloat3(&other->GetPosition());
			XMVECTOR vToOther = vOtherPos - vPos;
			float dSq = XMVectorGetX(XMVector3LengthSq(vToOther));

			if (dSq < separateRadius * separateRadius && dSq > 0.001f) {
				XMVECTOR pushAway = vPos - vOtherPos;
				pushAway = XMVector3Normalize(pushAway) / sqrtf(dSq); // 越近推力越大
				separationForce += pushAway;
			}
		}
		// 混合推力 (1.0 追击 + 1.5 排斥)
		dir = XMVector3Normalize(dir * 1.0f + separationForce * 1.5f);

		float moveSpeed = 1.0f; // 奔跑速度
		// 甚至可以在直线追击时跑得更快一点
		// moveSpeed = 1.5f; 

		XMVECTOR vNewPos = vPos + dir * moveSpeed * (float)elapsed_time;
		XMStoreFloat3(&m_pOwner->m_position, vNewPos);

		// 更新朝向
		float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
		m_pOwner->SetRotationY(angle);

		// 播放奔跑动画
		// if (g_pRunAnim) m_pOwner->m_Animator.PlayAnimation(g_pRunAnim, true, 0.2f);
		if (g_pWalkAnim) m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.2f);
	}
	else
	{
		// -----------------------------------------------------
		// 【情况 B：有障碍物】-> 启用 A* 寻路
		// -----------------------------------------------------

		m_RePathTimer -= (float)elapsed_time;

		// 需要重新寻路的情况：
		// 1. 路径是空的 (刚从直线状态切过来)
		// 2. 计时器到了
		if (m_pOwner->m_Path.empty() || m_RePathTimer <= 0.0f)
		{
			// 调用 A*
			m_pOwner->m_Path = Pathfinder::FindPath(m_pOwner->m_position, playerPos);
			m_pOwner->m_CurrentPathIndex = 0;

			// 稍微随机化寻路间隔
			m_RePathTimer = 0.5f + (rand() % 100) / 200.0f;
		}

		// --- 沿着路径点移动 (原有的 A* 移动代码) ---
		if (!m_pOwner->m_Path.empty())
		{
			if (m_pOwner->m_CurrentPathIndex >= m_pOwner->m_Path.size()) return;

			XMFLOAT3 targetNode = m_pOwner->m_Path[m_pOwner->m_CurrentPathIndex];

			XMVECTOR vPos = XMLoadFloat3(&m_pOwner->m_position);
			XMVECTOR vTarget = XMLoadFloat3(&targetNode);
			XMVECTOR toTarget = vTarget - vPos;
			toTarget = XMVectorSetY(toTarget, 0.0f);

			float dist = XMVectorGetX(XMVector3Length(toTarget));

			// 到达路点，切下一个
			if (dist < 0.5f) {
				m_pOwner->m_CurrentPathIndex++;
			}
			else {
				XMVECTOR dir = XMVector3Normalize(toTarget);

				XMVECTOR separationForce = XMVectorSet(0, 0, 0, 0);
				float separateRadius = 1.2f;

				for (EnemyTest* other : EnemyTest::g_AllEnemies) {
					if (other == m_pOwner || other->IsDestroyed()) continue;
					XMVECTOR vOtherPos = XMLoadFloat3(&other->GetPosition());
					XMVECTOR vToOther = vOtherPos - vPos; // 注意 vPos 需要在上面定义好
					float dSq = XMVectorGetX(XMVector3LengthSq(vToOther));

					if (dSq < separateRadius * separateRadius && dSq > 0.001f) {
						XMVECTOR pushAway = vPos - vOtherPos;
						pushAway = XMVector3Normalize(pushAway) / sqrtf(dSq);
						separationForce += pushAway;
					}
				}
				dir = XMVector3Normalize(dir * 1.0f + separationForce * 1.5f);

				float moveSpeed = 1.0f;
				XMVECTOR vNewPos = vPos + dir * moveSpeed * (float)elapsed_time;
				XMStoreFloat3(&m_pOwner->m_position, vNewPos);

				float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
				m_pOwner->SetRotationY(angle);

				if (g_pWalkAnim) m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.5f);
			}
		}
	}
		// 更新地面高度
		m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
}
void EnemyTest::EnemyTest_StateChase::Draw() const
{

}

bool EnemyTest::CanSeePlayer()
{
	// 1. 获取位置信息
	XMFLOAT3 playerPos = Player_GetPosition();
	XMFLOAT3 enemyPos = m_position;

	// 2. 计算 [敌人 -> 玩家] 的向量
	XMVECTOR vToPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&enemyPos);

	// 计算距离
	float distSq = XMVectorGetX(XMVector3LengthSq(vToPlayer));
	float range = m_DetectionRadius;

	// [检查 1] 距离检测：如果太远，直接看不见
	if (distSq > range * range) return false;

	// 3. 归一化方向向量 (变成长度为1的单位向量)
	vToPlayer = XMVector3Normalize(vToPlayer);

	// 4. 获取敌人的正前方向量
	// 假设 m_Rotation.y 是 Yaw 角 (绕Y轴旋转)
	// 注意：这里需要根据你的模型朝向微调，通常是 sin, 0, cos
	XMVECTOR vEnemyForward = XMVectorSet(sinf(m_Rotation.y), 0.0f, cosf(m_Rotation.y), 0.0f);

	// 5. 计算点积 (Dot Product)
	// Dot = cos(theta) * |A| * |B|。因为都是单位向量，所以 Dot = cos(theta)
	float dot = XMVectorGetX(XMVector3Dot(vEnemyForward, vToPlayer));

	// 6. 计算视锥阈值
	// m_FOVAngle 是总视野角度 (例如 90度)
	// 我们需要一半的角度 (45度) 的余弦值
	float halfFOV = XMConvertToRadians(m_FOVAngle * 0.5f);
	float threshold = cosf(halfFOV);

	// 7. [检查 2] 角度检测
	// 如果 dot > threshold，说明夹角比 halfFOV 小，也就是在扇形内
	if (dot > threshold)
	{
		// 玩家在扇形内，且在距离内！

		// --- 进阶预告：射线检测 (Raycast) ---
		// 将来我们要在这里加一个 "Collision_IntersectRayMap" 
		// 看看中间有没有墙。现在先默认没有墙。

		return true;
	}

	return false;
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



		// 3. 获取动画指针
		if (g_pIdleAnim == nullptr) g_pIdleAnim = g_pSkinningModel->GetDefaultAnimation();
		if (g_pWalkAnim == nullptr)   g_pWalkAnim = g_pSkinningModel->GetAnimation("Walk");
		if (g_pAttackAnim == nullptr) g_pAttackAnim = g_pSkinningModel->GetAnimation("Attack");
		if (g_pScreamAnim == nullptr) g_pScreamAnim = g_pSkinningModel->GetAnimation("Scream");
		if (g_pDyingAnim == nullptr)  g_pDyingAnim = g_pSkinningModel->GetAnimation("Dying");
		if (g_pReaction_HitAnim == nullptr) g_pReaction_HitAnim = g_pSkinningModel->GetAnimation("Reaction Hit");
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
	if (!m_bAlertedStatus) {
		m_bAlertedStatus = true; // 标记为已警觉

		// 切换到追逐状态 (这样下一帧 Update 就会开始跑向玩家)
		ChangeState(new EnemyTest_StateChase(this));
		if (g_pScreamAnim) {
			m_Animator.PlayAnimation(g_pScreamAnim, false, 0.1f);
		}

		m_HP -= damage; // 记得扣血
		return;
	}

	// =========================================================
	// 正常战斗状态
	// =========================================================
	m_HP -= damage;

	if (m_HP <= 0.0f) {
		// --- 死亡逻辑 ---
		m_HP = 0.0f;
		m_bIsDead = true;
		m_Animator.PlayAnimation(g_pDyingAnim, false, 0.1f);
		m_DeathTimer = 3.5f;

		int rate = rand() % 100; // 0 ~ 99
		if (rate < 30)
		{
			// 2. 决定掉落什么
			// 假设 ID 4 是子弹盒 (参照 Inventory.cpp 的定义)
			int dropItemID = 4;

			// 3. 在敌人当前位置生成
			DropItem_Spawn(m_position, dropItemID);
		}
	}
	else {
		// --- 存活时的受击反馈 ---

		// 需求 2 & 3: 只有近战才播放受击动画，枪击正常追击
		if (isMelee) {
			// 近战攻击：播放硬直动画
			m_Animator.PlayAnimation(g_pReaction_HitAnim, false, 0.1f);
		}
		else {
			// 枪击：什么都不做 (不播放 Hit 动画)
			// 这样敌人如果是 Chase 状态，就会继续播放 Run/Walk 动画追你
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


