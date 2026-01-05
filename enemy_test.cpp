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
using namespace DirectX;


const Animation* EnemyTest::g_pIdleAnim = nullptr;
const Animation* EnemyTest::g_pWalkAnim = nullptr;
const Animation* EnemyTest::g_pAttackAnim = nullptr;
const Animation* EnemyTest::g_pScreamAnim = nullptr;
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

	XMFLOAT3 playerPos = Player_GetPosition(); // 获取玩家坐标
	XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);
	XMVECTOR vEnemyPos = XMLoadFloat3(&m_pOwner->m_position);

	//视锥距离检测
	XMVECTOR toPlayerVec = vPlayerPos - vEnemyPos;
	float distSq = XMVectorGetX(XMVector3LengthSq(toPlayerVec));
	if (distSq < (m_pOwner->m_DetectionRadius * m_pOwner->m_DetectionRadius)) //
	{
		// 3. 角度检查（第二层过滤）

		// A. 计算敌人的正前方向量 (参考 PlayerCharacter.cpp 逻辑)
		float rotY = m_pOwner->m_Rotation.y; //
		XMVECTOR vForward = XMVectorSet(sinf(rotY), 0.0f, cosf(rotY), 0.0f);

		// B. 计算指向玩家的单位向量
		XMVECTOR vTargetDir = XMVector3Normalize(toPlayerVec);

		// C. 计算点积（得到夹角的余弦值 cosθ）
		// XMVector3Dot 返回的是向量，x分量存储结果
		float dotProduct = XMVectorGetX(XMVector3Dot(vForward, vTargetDir));

		// D. 计算视野阈值
		// 如果 FOV 是 90度，那么 HalfFOV 是 45度。我们需要 cos(45°)
		float halfFOVInRadians = XMConvertToRadians(m_pOwner->m_FOVAngle * 0.5f);
		float threshold = cosf(halfFOVInRadians);

		// E. 判定：如果 cosθ > cos(HalfFOV)，说明玩家在视野扇区内
		if (dotProduct > threshold)
		{
			// 发现玩家！
			m_pOwner->ChangeState(new EnemyTest_StateChase(m_pOwner)); //
			return;
		}
	}


	//听觉检测
	XMFLOAT3 soundPos;
	float soundRadius;
	if (Sound_GetLatest(soundPos, soundRadius)) {
		XMVECTOR vSoundPos = XMLoadFloat3(&soundPos);
		XMVECTOR toSound = vSoundPos - vEnemyPos;
		float distSq = XMVectorGetX(XMVector3LengthSq(toSound));

		if (distSq < (soundRadius * soundRadius)) {
	
			m_TargetPoint = soundPos;
			m_WaitTimer = 0.0f; // 停止当前的 Idle 等待

			// 播放警觉动作 (非循环)
			if (!m_bAlerted && !m_pOwner->m_Animator.IsPlaying(g_pScreamAnim)) {
				m_pOwner->m_Animator.PlayAnimation(g_pScreamAnim, false, 0.2f);
				m_bAlerted = true; 
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
	: m_pOwner(pOwner)
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
	}
	else
	{
		// ==========================================
		// 【状态 B：移动逻辑】
		// ==========================================

		// 播放行走动画
		if (g_pWalkAnim) {
			m_pOwner->m_Animator.PlayAnimation(g_pWalkAnim, true, 0.3f);
		}

		// 计算移动
		XMVECTOR dir = XMVector3Normalize(toPlayer);
		float moveSpeed = 1.0f; // 统一追逐速度
		XMVECTOR vNewPos = vEnemyPos + dir * moveSpeed * (float)elapsed_time;
		XMStoreFloat3(&m_pOwner->m_position, vNewPos);

		// 防止打滑：根据实际位移速度缩放动画
		m_pOwner->m_Animator.SetSpeedScale(moveSpeed / 1.0f);

		// 更新朝向
		float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
		m_pOwner->SetRotationY(angle);

		// 不在攻击范围内时，重置攻击计时器（可选，保证下次接触瞬间能攻击）
		m_pOwner->m_LastAttackTimer = m_pOwner->m_AttackCooldown; 
	}

	// --- 3. 退出条件与地形适配 ---

	// 放弃追逐：玩家跑得太远
	if (dist > m_pOwner->m_DetectionRadius * 1.5f)
	{
		m_pOwner->ChangeState(new EnemyTest_StatePatrol(m_pOwner));
		return; // 切换状态后立即返回
	}

	// 更新地面高度
	m_pOwner->m_position.y = MeshField_GetHeight(m_pOwner->m_position.x, m_pOwner->m_position.z);
}
void EnemyTest::EnemyTest_StateChase::Draw() const
{

}

EnemyTest::EnemyTest(const DirectX::XMFLOAT3& position)
	: m_position(position) 
{
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


		// 3. 获取动画指针
		if (g_pIdleAnim == nullptr) g_pIdleAnim = g_pSkinningModel->GetDefaultAnimation();
		if (g_pWalkAnim == nullptr)   g_pWalkAnim = g_pSkinningModel->GetAnimation("Walk");
		if (g_pAttackAnim == nullptr) g_pAttackAnim = g_pSkinningModel->GetAnimation("Attack");
		if (g_pScreamAnim == nullptr) g_pScreamAnim = g_pSkinningModel->GetAnimation("Scream");
	}

	// 3. 只取一次 Idle 动画指针，共享给所有敌人
	if (g_pIdleAnim == nullptr)
	{
		g_pIdleAnim = g_pSkinningModel->GetDefaultAnimation();
		// 或者，若你 Idle 不是默认动画：
		// g_pIdleAnim = g_pSkinningModel->GetAnimation("Zombie Idle1");
	}
}

void EnemyTest::UnloadAssets()
{
	if (g_pSkinningModel != nullptr) {
		g_pSkinningModel->Release();
		delete g_pSkinningModel;
		g_pSkinningModel = nullptr;
	}
}

AABB EnemyTest::GetAABB()
{
	float hw = 0.5f; // 半宽
	float h = 2.0f;  // 高度

	return {
		{ m_position.x - hw, m_position.y,        m_position.z - hw },
		{ m_position.x + hw, m_position.y + h,    m_position.z + hw }
	};

	
	return {
		{m_position.x - 1.0f, m_position.y, m_position.z - 1.0f},
		{m_position.x + 1.0f, m_position.y + 2.0f, m_position.z + 1.0f}
	};
}

void EnemyTest::ChangeState(State* pNextState)
{
	Enemy::ChangeState(pNextState);
}
void EnemyTest::ApplyKnockback(const DirectX::XMVECTOR& direction, float force)
{
	// 如果已经死亡，不处理物理效果
	if (m_bIsDestroyed) return;

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

	// 5. [可选] 播放受击/尖叫动作造成硬直
	// 只有当前不在播放尖叫时才播放，避免鬼畜
	if (g_pScreamAnim && !m_Animator.IsPlaying(g_pScreamAnim)) {
		m_Animator.PlayAnimation(g_pScreamAnim, false, 0.1f);
	}
}
void EnemyTest::SetAlerted(bool alerted) { m_bAlertedStatus = alerted; }


void EnemyTest::Update(double elapsed_time)
{
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


