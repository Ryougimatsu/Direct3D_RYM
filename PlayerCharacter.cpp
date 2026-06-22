#include "PlayerCharacter.h"
#include "SkinningShader.h"
#include "Player_Camera.h"
#include "direct3d.h"
#include "texture.h"
#include "billboard.h"
#include "bullet.h"
#include "enemy.h"
#include "key_logger.h"
#include "Pathfinder.h"
#include "game.h"
#include "Shader_Shadow.h"
#include "model.h"
#include "shader_billboard.h"
#include "map.h"
#include "sprite.h"
#include <utility>
using namespace DirectX;

// 全局单例指针
PlayerCharacter* g_pPlayerInstance = nullptr;

// ======================================================================================
// 匿名命名空间：内部使用的常量、静态变量与辅助函数
// ======================================================================================
namespace {
	// --- 枪械姿态配置（枪附着在手骨时的初始角度修正）---
	float m_GunPitch = DirectX::XMConvertToRadians(5.0f);   // 绕 X 轴旋转（上下抬头）
	float m_GunYaw   = DirectX::XMConvertToRadians(-90.0f); // 绕 Y 轴旋转（左右偏航）
	float m_GunRoll  = DirectX::XMConvertToRadians(90.0f);  // 绕 Z 轴旋转（枪身横滚）

	DirectX::XMFLOAT3 m_GunOffset = { 0.05f, 0.05f, 0.0f };       // 枪相对于手骨的本地位移偏移（手心微调）
	DirectX::XMFLOAT3 m_MuzzleLocalOffset = { 0.8f, 0.05f, 0.0f }; // 枪口相对于枪根的本地坐标偏移

	// --- 受伤闪红效果 ---
	float g_DamageFlashAlpha = 0.0f; // 受伤红屏的当前透明度（每帧自动衰减）
	int   g_DamageTexID = -1;        // 红屏全屏 Sprite 所用纹理 ID

	// --- 激光瞄准线配置 ---
	float m_LaserLength = 20.0f; // 激光最大射程（单位：游戏米），超出此距离截断
	int   m_LaserTexID = -1;     // 激光线条所用纹理 ID

	// --- 共享资源（同一场景内所有 PlayerCharacter 实例共享）---
	SkinningModel* g_pSharedPlayerModel = nullptr; // 角色骨骼蒙皮模型（含所有动作剪辑）
	MODEL*         g_pSharedGunModel    = nullptr; // 枪械静态模型

	// --- 声音事件系统（供敌人 AI 侦听枪声）---
	DirectX::XMFLOAT3 g_LastSoundPos = { 0, 0, 0 }; // 最近一次声音事件的世界坐标
	float g_SoundIntensity = 0.0f;                   // 声音强度（等效侦测半径，越大越容易被发现）
	float g_SoundTimer     = 0.0f;                   // 声音事件剩余有效时间（倒计时至 0 后失效）

	// --- 输入辅助函数 ---
	DirectX::XMVECTOR GetInputVector() {
		using namespace DirectX;
		XMVECTOR input = XMVectorSet(0, 0, 0, 0);

		// 检测键盘状态 (仅当窗口激活时)
		if (GetForegroundWindow() == Direct3D_GetWindowHandle()) {
			if (GetAsyncKeyState('W')) input = XMVectorAdd(input, XMVectorSet(0, 0, 1, 0));  // 前
			if (GetAsyncKeyState('S')) input = XMVectorAdd(input, XMVectorSet(0, 0, -1, 0)); // 后
			if (GetAsyncKeyState('A')) input = XMVectorAdd(input, XMVectorSet(-1, 0, 0, 0)); // 左
			if (GetAsyncKeyState('D')) input = XMVectorAdd(input, XMVectorSet(1, 0, 0, 0));  // 右
		}
		if (XMVector3Greater(XMVector3LengthSq(input), XMVectorZero())) {
			return XMVector3Normalize(input);
		}

		return XMVectorZero();
	}

	// --- 动画选择辅助函数 ---
	// 根据移动向量与角色朝向之间的夹角，选择对应的 8 向战术移动动画。
	// angle 为移动方向相对于角色正前方的偏角（弧度），范围 [-π, π]。
	// 将圆周均分为 8 个 45° 扇区，每个扇区对应一个方向性移动动画。
	std::string SelectTacticalAnim(float angle) {
		using namespace DirectX;
		float degrees = XMConvertToDegrees(angle); // 转换为角度，范围: -180 到 180

		if (degrees >= -22.5f && degrees < 22.5f)    return "Walk Forward";        // 正前方 ±22.5°
		if (degrees >= 22.5f && degrees < 67.5f)     return "Walk Forward Right";  // 右前方
		if (degrees >= 67.5f && degrees < 112.5f)    return "Walk Right";           // 正右方
		if (degrees >= 112.5f && degrees < 157.5f)   return "Walk Backward Right"; // 右后方
		if (degrees >= 157.5f || degrees < -157.5f)  return "Walk Backward";       // 正后方 ±22.5°
		if (degrees >= -157.5f && degrees < -112.5f) return "Walk Backward Left";  // 左后方
		if (degrees >= -112.5f && degrees < -67.5f)  return "Walk Left";            // 正左方
		if (degrees >= -67.5f && degrees < -22.5f)   return "Walk Forward Left";   // 左前方

		return "Rifle Aiming Idle"; // 兜底：保持持枪待机
	}

	// 从枪口出发，沿枪管方向发射射线，检测最近的碰撞点。
	// 依次测试地图墙壁和敌人 AABB，返回实际截断距离（≤ maxDistance）。
	float CalculateLaserDistance(const DirectX::XMFLOAT3& startPos, const DirectX::XMFLOAT3& direction, float maxDistance) {
		Ray laserRay;
		laserRay.origin    = startPos;
		laserRay.direction = direction;

		float closestDist = maxDistance; // 默认射到最大射程

		// 1. 检测地图障碍物 (墙壁等)
		const auto& mapObjects = Map_GetObjects();
		for (const auto& obj : mapObjects) {
			if (obj.KindId != MAP_KIND_WALL) continue; // 只检测墙壁
			float hitDist = 0.0f;
			if (Collision_IntersectRayAABB(laserRay, obj.Aabb, hitDist)) {
				if (hitDist > 0.0f && hitDist < closestDist) {
					closestDist = hitDist;
				}
			}
		}

		// 2. 检测敌人
		for (int i = 0; i < Enemy_GetEnemyCount(); i++) {
			Enemy* pEnemy = Enemy_GetEnemy(i);
			if (!pEnemy) continue;
			float hitDist = 0.0f;
			if (Collision_IntersectRayAABB(laserRay, pEnemy->GetAABB(), hitDist)) {
				if (hitDist > 0.0f && hitDist < closestDist) {
					closestDist = hitDist;
				}
			}
		}

		return closestDist;
	}

	DirectX::XMMATRIX FixGunMatrix(DirectX::XMMATRIX gunWorld) {
		using namespace DirectX;

		// 1. 提取当前的枪管朝向 (X轴方向)
		XMVECTOR currentFwd = XMVector3Normalize(gunWorld.r[0]);

		// 2. 计算目标朝向：把当前朝向压平到 XZ 水平面
		XMVECTOR targetFwd = XMVectorSetY(currentFwd, 0.0f);

		// 如果枪几乎垂直朝天或朝地，直接返回原样防止数学错误
		if (XMVectorGetX(XMVector3LengthSq(targetFwd)) < 0.0001f) {
			return gunWorld;
		}
		targetFwd = XMVector3Normalize(targetFwd);

		// 3. 计算从 currentFwd 旋转到 targetFwd 所需的“旋转轴”和“角度”
		XMVECTOR rotAxis = XMVector3Cross(currentFwd, targetFwd);
		float axisLen = XMVectorGetX(XMVector3Length(rotAxis));

		// 只有当枪管不在水平面时，才需要进行修正
		if (axisLen > 0.0001f) {
			rotAxis = XMVector3Normalize(rotAxis);
			float dotVal = XMVectorGetX(XMVector3Dot(currentFwd, targetFwd));

			// 限制范围，防止浮点误差导致 acosf 崩溃
			if (dotVal < -1.0f) dotVal = -1.0f;
			if (dotVal > 1.0f) dotVal = 1.0f;
			float angle = acosf(dotVal);

			// 构建修正旋转矩阵
			XMMATRIX rotMat = XMMatrixRotationAxis(rotAxis, angle);

			// 4. 将旋转应用到整把枪上（只旋转姿态，不改变位置）
			XMVECTOR pos = gunWorld.r[3];             // 暂存枪的位置
			gunWorld.r[3] = XMVectorSet(0, 0, 0, 1);  // 消除位置影响
			gunWorld = gunWorld * rotMat;             // 整体应用旋转
			gunWorld.r[3] = pos;                      // 恢复枪的位置
		}

		return gunWorld;
	}
}

// 内部使用的声音发射函数 (非类成员)
void Player_EmitSound(const DirectX::XMFLOAT3& pos, float radius) {
	g_LastSoundPos = pos;
	g_SoundIntensity = radius;
	g_SoundTimer = 0.5f; // 声音在空气中存在 0.5 秒
}


// ======================================================================================
// 类实现：PlayerCharacter
// ======================================================================================

// ----------------------------------------------------------------
// 1. 生命周期 (Lifecycle)
// ----------------------------------------------------------------
PlayerCharacter::PlayerCharacter(ExperienceConfig experienceConfig)
	: m_Experience(std::move(experienceConfig))
{
}

PlayerCharacter::~PlayerCharacter() {
	// 释放枪口粒子系统
	if (m_pMuzzleFireSystem) {
		m_pMuzzleFireSystem->Finalize();
		delete m_pMuzzleFireSystem;
		m_pMuzzleFireSystem = nullptr;
	}

	// 清除全局单例指针，防止其他模块访问已销毁的实例（悬空指针）
	if (g_pPlayerInstance == this) {
		g_pPlayerInstance = nullptr;
	}
}

void PlayerCharacter::LoadAssets()
{
	// 如果模型已经存在，直接返回，不再重复加载
	if (g_pSharedPlayerModel) return;

	g_pSharedPlayerModel = new SkinningModel();
	g_pSharedPlayerModel->Load("resource/model/Idle.fbx", 1.0f); // 基础模型（T-Pose，含骨骼信息）

	// 加载所有战术移动与战斗动作剪辑
	g_pSharedPlayerModel->LoadAnimation("Walk Forward", "resource/model/Character_Model/Walk Forward.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Walk Backward", "resource/model/Character_Model/Walk Backward.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Walk Left", "resource/model/Character_Model/Walk Left.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Walk Right", "resource/model/Character_Model/Walk Right.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Walk Forward Right", "resource/model/Character_Model/Walk Forward Right.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Walk Forward Left", "resource/model/Character_Model/Walk Forward Left.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Walk Backward Right", "resource/model/Character_Model/Walk Backward Right.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Walk Backward Left", "resource/model/Character_Model/Walk Backward Left.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Firing Rifle", "resource/model/Character_Model/Firing Rifle.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Firing Rifle Idle", "resource/model/Character_Model/Firing Rifle idle.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Slash Advance", "resource/model/Character_Model/Slash Advance.fbx", 1.0f);
	g_pSharedPlayerModel->LoadAnimation("Rifle Death", "resource/model/Character_Model/Rifle Death.fbx", 1.0f);

	g_pSharedGunModel = ModelLoad("resource/model/M4a4.fbx", 1.0f);
}

void PlayerCharacter::UnloadAssets()
{
	if (g_pSharedPlayerModel) {
		g_pSharedPlayerModel->Release();
		delete g_pSharedPlayerModel;
		g_pSharedPlayerModel = nullptr;
	}
	if (g_pSharedGunModel) {
		ModelRelease(g_pSharedGunModel);
		g_pSharedGunModel = nullptr;
	}
}

bool PlayerCharacter::Initialize() {
	g_pPlayerInstance = this;

	// 确保资源已加载
	LoadAssets();

	// 直接指向共享资源
	m_pModel = g_pSharedPlayerModel;
	m_pGunModel = g_pSharedGunModel;

	// 初始化组件
	m_Animator.PlayAnimation(m_pModel->GetDefaultAnimation(), true);
	m_LaserTexID = Texture_LoadFromFile(L"resource/texture/Laser001.png");
	Billboard_Initialize();

	// 初始化枪口火焰粒子系统
	m_MuzzleFireTexID = Texture_LoadFromFile(L"resource/texture/kenney_particle-pack/PNG (Transparent)/flame_01.png");
	m_pMuzzleFireSystem = new ParticleSystem();
	m_pMuzzleFireSystem->Initialize(500, m_MuzzleFireTexID);

	g_DamageTexID = Texture_LoadFromFile(L"resource/texture/white.png");

	return true;
}

// ----------------------------------------------------------------
// 2. 核心循环 (Update & Draw)
// ----------------------------------------------------------------
void PlayerCharacter::Update(double dt) {

	// --- 死亡逻辑处理 ---
	if (m_CurrentState == CharacterState::Dead)
	{
		m_DeathTimer += (float)dt;
		m_Animator.Update(dt);

		if (m_DeathTimer >= 4.0f) {
			m_IsDeadFinished = true; // 标记为“彻底死亡”
		}
		return;
	}

	if (m_HP <= 0.0f)
	{
		m_CurrentState = CharacterState::Dead;
		m_Animator.PlayAnimation(m_pModel->GetAnimation("Rifle Death"), false, 0.1f);
		m_DeathTimer = 0.0f;
		return;
	}

	// --- 准备数据 ---
	XMMATRIX view = XMLoadFloat4x4(&Player_Camera_GetViewMatrix());
	XMMATRIX proj = XMLoadFloat4x4(&Player_Camera_GetProjectionMatrix());

	// --- 1. 旋转逻辑：始终指向鼠标 ---
	if (m_MeleeTimer <= 0.2f)
	{
		XMVECTOR mousePos = GetMouseWorldPos(view, proj);
		XMVECTOR playerPos = XMLoadFloat3(&m_Position);
		XMVECTOR lookDir = XMVectorSubtract(mousePos, playerPos);

		float targetAngle = atan2f(XMVectorGetX(lookDir), XMVectorGetZ(lookDir));
		float angleDiff = targetAngle - m_RotationY;
		while (angleDiff < -XM_PI) angleDiff += XM_2PI;
		while (angleDiff > XM_PI) angleDiff -= XM_2PI;
		m_RotationY += angleDiff * 0.15f;
	}

	// --- 2. 输入平滑处理 ---
	XMVECTOR inputVec = GetInputVector();
	XMVECTOR currentSmoothed = XMLoadFloat3(&m_CurrentMoveDir);
	XMVECTOR smoothedInput = XMVectorLerp(currentSmoothed, inputVec, 8.0f * (float)dt);
	XMStoreFloat3(&m_CurrentMoveDir, smoothedInput);

	float moveLen = XMVectorGetX(XMVector3Length(smoothedInput));
	bool isFiring = (GetAsyncKeyState(VK_LBUTTON) & 0x8000);

	// --- 计时器更新 ---
	if (m_MeleeTimer > 0.0f) m_MeleeTimer -= (float)dt;
	if (m_ShootTimer > 0.0f) m_ShootTimer -= (float)dt;
	if (g_SoundTimer > 0.0f) g_SoundTimer -= (float)dt;
	if (m_InvincibleTimer > 0.0f) {
		m_InvincibleTimer -= (float)dt;
		if (m_InvincibleTimer < 0.0f) m_InvincibleTimer = 0.0f;
	}

	if (g_DamageFlashAlpha > 0.0f) {
		g_DamageFlashAlpha -= (float)dt * 1.5f; // 衰减速度，越大消失越快
		if (g_DamageFlashAlpha < 0.0f) g_DamageFlashAlpha = 0.0f;
	}

	// --- 换弹逻辑 (R键) ---
	if (KeyLogger_IsTrigger(KK_R) && !m_IsReloading && m_CurrentAmmo < MAG_SIZE && m_TotalAmmo > 0) {
		m_IsReloading = true;
		m_ReloadTimer = RELOAD_TIME;
	}

	if (m_IsReloading) {
		m_ReloadTimer -= (float)dt;
		if (m_ReloadTimer <= 0.0f) {
			m_IsReloading = false;
			int needed = MAG_SIZE - m_CurrentAmmo;
			int actualFill = (m_TotalAmmo >= needed) ? needed : m_TotalAmmo;
			m_CurrentAmmo += actualFill;
			m_TotalAmmo -= actualFill;
		}
	}

	// --- 3. 近战触发逻辑 (优先级最高) ---
	if ((GetAsyncKeyState('F') & 0x8000) && m_MeleeTimer <= 0.0f) {
		XMVECTOR forward = XMVectorSet(sinf(m_RotationY), 0, cosf(m_RotationY), 0);

		// 造成伤害
		Enemy_ApplyMeleeDamage(m_Position, forward, 2.5f, 120.0f);

		// 播放动画
		m_Animator.PlayAnimation(m_pModel->GetAnimation("Slash Advance"), false, 0.05f);
		m_Animator.SetSpeedScale(2.5f);

		// 设置硬直
		m_MeleeTimer = 0.8f;
	}

	// --- 4. 动画状态机 & 移动逻辑 ---
	bool isInMeleeAnimation = (m_MeleeTimer > 0.2f); // 动作锁

	if (!isInMeleeAnimation)
	{
		std::string animToPlay = "Rifle Aiming Idle";
		float crossfadeTime = 0.2f;

		// A. 移动检测与动画选择
		if (moveLen > 0.05f) {
			XMVECTOR forward = XMVectorSet(sinf(m_RotationY), 0, cosf(m_RotationY), 0);
			XMVECTOR right = XMVectorSet(cosf(m_RotationY), 0, -sinf(m_RotationY), 0);
			float fwdDot = XMVectorGetX(XMVector3Dot(forward, smoothedInput));
			float sideDot = XMVectorGetX(XMVector3Dot(right, smoothedInput));
			float angle = atan2f(sideDot, fwdDot);

			animToPlay = SelectTacticalAnim(angle);
			if (isFiring) animToPlay = "Firing Rifle";
			m_Animator.SetSpeedScale(moveLen);
		}
		else {
			if (isFiring) animToPlay = "Firing Rifle Idle";
			else animToPlay = "Rifle Aiming Idle";
			m_Animator.SetSpeedScale(1.0f);
		}

		m_Animator.PlayAnimation(m_pModel->GetAnimation(animToPlay), true, crossfadeTime);

		// B. 物理位移 (碰撞检测)
		XMVECTOR moveVec = smoothedInput * m_MoveSpeed * (float)dt;
		XMFLOAT3 moveDelta;
		XMStoreFloat3(&moveDelta, moveVec);

		// X 轴移动尝试
		float oldX = m_Position.x;
		m_Position.x += moveDelta.x;
		if (Game_CheckCollisionWithWalls(this->GetAABB())) m_Position.x = oldX;

		// Z 轴移动尝试
		float oldZ = m_Position.z;
		m_Position.z += moveDelta.z;
		if (Game_CheckCollisionWithWalls(this->GetAABB())) m_Position.z = oldZ;

		// C. 开火逻辑（条件：鼠标左键按住 + 射速冷却结束 + 未换弹 + 有子弹）
		if (isFiring && m_ShootTimer <= 0.0f && !m_IsReloading && m_CurrentAmmo > 0) {
			const auto& nameMap = m_pModel->GetSkeleton().nameToIndex;
			if (nameMap.count("mixamorig:RightHand")) {
				int handIdx = nameMap.at("mixamorig:RightHand");
				XMMATRIX handMat = m_Animator.GetBoneGlobalMatrix(handIdx); // 从 Animator 取当前帧的右手骨世界矩阵

				// 重新构造与 Draw 完全一致的枪世界矩阵（gunLocal × handMat × worldMatrix）
				XMMATRIX world    = XMMatrixScaling(m_Scale, m_Scale, m_Scale) * XMMatrixRotationY(m_RotationY + XM_PI) * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
				XMMATRIX gunLocal = XMMatrixScaling(m_GunScale, m_GunScale, m_GunScale) * XMMatrixRotationRollPitchYaw(m_GunPitch, m_GunYaw, m_GunRoll) * XMMatrixTranslation(m_GunOffset.x, m_GunOffset.y, m_GunOffset.z);
				XMMATRIX gunWorld = gunLocal * handMat * world;
				gunWorld = FixGunMatrix(gunWorld); // 修正枪管俯仰，保持水平

				// 将枪口本地偏移变换到世界空间，得到枪口基础位置
				XMVECTOR muzzleLocalV = XMLoadFloat3(&m_MuzzleLocalOffset);
				XMVECTOR basePos      = XMVector3TransformCoord(muzzleLocalV, gunWorld);

				// 必须 Normalize：gunWorld.r[0] 因 m_Scale(0.01) 而长度约 0.01，不归一化偏移量极小
				XMVECTOR gunForwardDir = XMVector3Normalize(gunWorld.r[0]);

				// 再向枪管方向前进 1.0f，使子弹/粒子从真正的枪口发出（与 Draw 中激光起点保持一致）
				XMVECTOR actualMuzzlePos = basePos + (gunForwardDir * 1.0f);

				XMFLOAT3 p, v;
				XMStoreFloat3(&p, actualMuzzlePos);
				XMStoreFloat3(&v, gunForwardDir);

				// 生成子弹：从枪口位置沿枪管方向飞出
				Bullet_Create(p, v);
				m_CurrentAmmo--;

				// 粒子发射位置在枪口上方微调（视觉修正）
				XMFLOAT3 flashPos = p;
				flashPos.y += 0.2f;
				m_pMuzzleFireSystem->EmitMuzzleFire(flashPos, v, 15); // 发射 15 个枪口火焰粒子

				Player_Camera_AddShake(0.1f);       // 开枪轻微震屏
				Player_EmitSound(m_Position, 25.0f); // 广播声音事件（半径 25 米，供敌人 AI 侦听）
				m_ShootTimer = m_FireRate;           // 重置射速冷却
			}
		}
	}

	// --- 5. 动画更新 ---
	m_Animator.Update(dt);

	// --- 6. 粒子系统更新 ---
	m_pMuzzleFireSystem->Update(dt);
}

void PlayerCharacter::Draw(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj) {
	if (!m_pModel) return;

	// 1. 设置着色器全局矩阵
	SkinningShader_3D_SetViewMatrix(view);
	SkinningShader_3D_SetProjectMatrix(proj);

	Shader_Billboard_SetViewMatrix(view);
	Shader_Billboard_SetProjectMatrix(proj);

	// 2. 构造世界矩阵
	DirectX::XMMATRIX world = DirectX::XMMatrixScaling(m_Scale, m_Scale, m_Scale) * DirectX::XMMatrixRotationY(m_RotationY + XM_PI) * DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	SkinningShader_3D_SetWorldMatrix(world);

	// 3. 骨骼传输
	auto bones = m_Animator.GetFinalBoneMatrices(m_pModel->GetSkeleton());
	SkinningShader_3D_SetBoneTransforms(bones);

	// 4. 绘制角色
	SkinningShader_3D_Begin();
	m_pModel->Draw();

	// 5. 绘制武器与激光（近战动画期间隐藏武器，死亡后也隐藏）
	if (m_MeleeTimer <= 0.2f && m_CurrentState != CharacterState::Dead)
	{
		const auto& nameMap = m_pModel->GetSkeleton().nameToIndex;
		if (nameMap.count("mixamorig:RightHand")) {
			int handIdx = nameMap.at("mixamorig:RightHand");
			XMMATRIX handMat = m_Animator.GetBoneGlobalMatrix(handIdx); // 取当前帧右手骨世界矩阵

			// 构造枪的世界矩阵：gunLocal（姿态修正）× handMat（骨骼动画）× world（角色变换）
			XMMATRIX gunLocal =
				XMMatrixScaling(m_GunScale, m_GunScale, m_GunScale) *
				XMMatrixRotationRollPitchYaw(m_GunPitch, m_GunYaw, m_GunRoll) *
				XMMatrixTranslation(m_GunOffset.x, m_GunOffset.y, m_GunOffset.z);

			XMMATRIX gunWorld = gunLocal * handMat * world;
			gunWorld = FixGunMatrix(gunWorld); // 修正枪管俯仰角，始终保持枪管水平

			// 绘制枪械静态模型
			ModelDraw(m_pGunModel, gunWorld);

			// 计算枪口世界坐标（枪口本地偏移 → 世界空间）
			XMVECTOR muzzleLocalV  = XMLoadFloat3(&m_MuzzleLocalOffset);
			XMVECTOR basePos       = XMVector3TransformCoord(muzzleLocalV, gunWorld);
			XMVECTOR gunForwardDir = XMVector3Normalize(gunWorld.r[0]); // 归一化枪管方向

			// 激光起点直接从枪口出发（basePos），不额外前移
			XMVECTOR actualMuzzlePos = basePos;
			DirectX::XMFLOAT3 startF3, dirF3;
			DirectX::XMStoreFloat3(&startF3, actualMuzzlePos);
			DirectX::XMStoreFloat3(&dirF3, gunForwardDir);

			// 射线检测截断：遇到墙壁或敌人时缩短激光
			float actualLength   = CalculateLaserDistance(startF3, dirF3, m_LaserLength);
			XMVECTOR laserEndPos = actualMuzzlePos + (gunForwardDir * actualLength);

			// 切换到加法混合（ADD）：让激光/粒子叠加发光而非遮挡背景
			Direct3D_SetBlendState(BLEND_MODE_ADD);
			// 保持深度测试（防止激光透视穿模），但关闭深度写入（透明物体不写深度）
			Direct3D_SetDepthEnable(true);
			Direct3D_SetDepthStencilStateDepthWriteDisable(false);

			// 绘制激光线（Billboard 渲染，宽度 1.5f，白色）
			Laser_Billboard_Draw(m_LaserTexID, actualMuzzlePos, laserEndPos, 1.5f, view, { 1.0f, 1.0f, 1.0f, 1.0f });

			// 绘制枪口火焰粒子（与激光同一混合状态下绘制）
			m_pMuzzleFireSystem->Draw();

			// 恢复默认渲染状态
			Direct3D_SetDepthStencilStateDepthWriteDisable(true); // 恢复深度写入
			Direct3D_SetDepthEnable(true);
			Direct3D_SetBlendState(BLEND_MODE_NONE);
		}
	}
}

void PlayerCharacter::DrawShadow(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj)
{
	if (!m_pModel) return;

	// ==========================================================
	// 1. 绘制玩家主体 (使用新的骨骼阴影 Shader)
	// ==========================================================
	Shader_Shadow_ApplySkinning();

	// 传递并设置世界矩阵 (Shader_Shadow 会自动结合 LightView 和 LightProj 计算并更新 b0)
	DirectX::XMMATRIX world = DirectX::XMMatrixScaling(m_Scale, m_Scale, m_Scale) * DirectX::XMMatrixRotationY(m_RotationY + DirectX::XM_PI) * DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	Shader_Shadow_SetWorldMatrix(world);

	// 获取当前动画的骨骼矩阵并更新到显存 (这会更新 g_pCBBones)
	auto bones = m_Animator.GetFinalBoneMatrices(m_pModel->GetSkeleton());
	SkinningShader_3D_SetBoneTransforms(bones);

	// 手动将骨骼 Buffer 绑定到 Vertex Shader 的槽位 1 (对应 hlsl 里的 register(b1))
	ID3D11Buffer* pBoneBuffer = SkinningShader_3D_GetBoneBuffer();
	Direct3D_GetDeviceContext()->VSSetConstantBuffers(1, 1, &pBoneBuffer);

	// 绘制模型
	m_pModel->Draw();

	// ==========================================================
	// 2. 绘制枪械 (恢复静态阴影 Shader)
	// ==========================================================
	if (m_MeleeTimer <= 0.2f && m_pGunModel && m_CurrentState != CharacterState::Dead)
	{
		// 切换回静态物体的 Shadow Shader
		Shader_Shadow_Apply();

		const auto& nameMap = m_pModel->GetSkeleton().nameToIndex;
		if (nameMap.count("mixamorig:RightHand"))
		{
			int handIdx = nameMap.at("mixamorig:RightHand");
			DirectX::XMMATRIX handMat = m_Animator.GetBoneGlobalMatrix(handIdx);
			DirectX::XMMATRIX gunLocal =
				DirectX::XMMatrixScaling(m_GunScale, m_GunScale, m_GunScale) *
				DirectX::XMMatrixRotationRollPitchYaw(m_GunPitch, m_GunYaw, m_GunRoll) *
				DirectX::XMMatrixTranslation(m_GunOffset.x, m_GunOffset.y, m_GunOffset.z);

			DirectX::XMMATRIX gunWorld = gunLocal * handMat * world;
			gunWorld = FixGunMatrix(gunWorld);
			// 静态模型画阴影
			ModelDrawShadow(m_pGunModel, gunWorld);
		}
	}
}

// ----------------------------------------------------------------
// 3. 战斗与属性 (Combat & Attributes)
// ----------------------------------------------------------------
void PlayerCharacter::ApplyDamage(float damage)
{
	if (m_InvincibleTimer > 0.0f) return;

	m_HP -= damage;
	m_InvincibleTimer = m_InvincibleDuration;

	Player_Camera_AddShake(0.3f); // 受伤震动稍微大一点
	g_DamageFlashAlpha = 0.5f;    // 屏幕闪红初始透明度 (0.5 表示半透明)
}

void PlayerCharacter::Heal(float amount)
{
	if (m_HP <= 0.0f) return;

	m_HP += amount;
	if (m_HP > m_MaxHP) {
		m_HP = m_MaxHP;
	}
}

void PlayerCharacter::AddAmmo(int amount)
{
	m_TotalAmmo += amount;
	if (m_TotalAmmo > 300) m_TotalAmmo = 300;
}

ExperienceGainResult PlayerCharacter::AddExperience(
	std::uint64_t enemyBaseExperience,
	std::uint32_t enemyLevel)
{
	return m_Experience.AddExperience(enemyBaseExperience, enemyLevel);
}

AABB PlayerCharacter::GetAABB() const {
	float halfWidth = 0.5f;
	float height = 1.8f;

	return {
		{ m_Position.x - halfWidth, m_Position.y,          m_Position.z - halfWidth },
		{ m_Position.x + halfWidth, m_Position.y + height, m_Position.z + halfWidth }
	};
}


// ======================================================================================
// 全局 C 风格接口（供其他模块通过单例访问玩家功能，内部均做空指针检查）
// ======================================================================================

// 返回当前存活的 PlayerCharacter 单例（场景切换或玩家死亡析构后返回 nullptr）
PlayerCharacter* Player_GetInstance() {
	return g_pPlayerInstance;
}

// 查询最近一次枪声事件：返回 true 时写入位置和范围，供敌人 AI 侦听
bool Sound_GetLatest(DirectX::XMFLOAT3& outPos, float& outRadius) {
	if (g_SoundTimer > 0.0f) {
		outPos    = g_LastSoundPos;
		outRadius = g_SoundIntensity;
		return true;
	}
	return false;
}

// 在屏幕上绘制受伤红色半透明全屏覆盖（每帧由 HUD 调用，Alpha 由 Update 自动衰减）
void Player_DrawDamageFlash() {
	if (g_DamageFlashAlpha > 0.0f && g_DamageTexID != -1) {
		float sw = (float)Direct3D_GetBackBufferWidth();
		float sh = (float)Direct3D_GetBackBufferHeight();
		Sprite_Draw(g_DamageTexID, 0.0f, 0.0f, sw, sh, { 1.0f, 0.0f, 0.0f, g_DamageFlashAlpha });
	}
}

// 获取玩家世界坐标（单例不存在时返回原点）
DirectX::XMFLOAT3 Player_GetPosition() {
	if (g_pPlayerInstance) {
		return g_pPlayerInstance->GetPosition();
	}
	return { 0.0f, 0.0f, 0.0f };
}

// 强制设置玩家位置（用于传送、出生点重置等）
void Player_SetPosition(const DirectX::XMFLOAT3& pos) {
	if (g_pPlayerInstance) {
		g_pPlayerInstance->SetPosition(pos);
	}
}

// 对玩家造成指定伤害（内部有无敌帧判断）
void Player_Damage(float damage) {
	if (g_pPlayerInstance) {
		g_pPlayerInstance->ApplyDamage(damage);
	}
}

// 为玩家增加备弹（超过上限 300 时截断）
void Player_AddAmmo(int count) {
	if (g_pPlayerInstance) {
		g_pPlayerInstance->AddAmmo(count);
	}
}

// 为玩家回血（死亡状态下无效，不超过最大血量）
void Player_Heal(float amount) {
	if (g_pPlayerInstance) {
		g_pPlayerInstance->Heal(amount);
	}
}
