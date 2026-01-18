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

using namespace DirectX;

// 全局单例指针
PlayerCharacter* g_pPlayerInstance = nullptr;

// ======================================================================================
// 匿名命名空间：内部使用的常量、静态变量与辅助函数
// ======================================================================================
namespace {
	// --- 枪械与瞄准配置 ---
	float m_GunPitch = DirectX::XMConvertToRadians(5.0f);   // 绕 X
	float m_GunYaw = DirectX::XMConvertToRadians(-90.0f); // 绕 Y
	float m_GunRoll = DirectX::XMConvertToRadians(90.0f);  // 绕 Z

	DirectX::XMFLOAT3 m_GunOffset = { 0.05f, 0.05f, 0.0f }; // 手心里的偏移
	DirectX::XMFLOAT3 m_MuzzleLocalOffset = { 0.0f, 0.0f, 0.5f };

	float m_LaserLength = 20.0f;
	int   m_LaserTexID = -1;

	// --- 资源共享 ---
	SkinningModel* g_pSharedPlayerModel = nullptr;
	MODEL* g_pSharedGunModel = nullptr;

	// --- 声音系统变量 ---
	DirectX::XMFLOAT3 g_LastSoundPos = { 0, 0, 0 }; // 声音位置
	float g_SoundIntensity = 0.0f;                  // 声音强度（半径）
	float g_SoundTimer = 0.0f;                      // 声音持续时间（秒）

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
	std::string SelectTacticalAnim(float angle) {
		using namespace DirectX;
		float degrees = XMConvertToDegrees(angle); // 范围: -180 到 180

		if (degrees >= -22.5f && degrees < 22.5f)   return "Walk Forward";
		if (degrees >= 22.5f && degrees < 67.5f)    return "Walk Forward Right";
		if (degrees >= 67.5f && degrees < 112.5f)   return "Walk Right";
		if (degrees >= 112.5f && degrees < 157.5f)  return "Walk Backward Right";
		if (degrees >= 157.5f || degrees < -157.5f) return "Walk Backward";
		if (degrees >= -157.5f && degrees < -112.5f) return "Walk Backward Left";
		if (degrees >= -112.5f && degrees < -67.5f)  return "Walk Left";
		if (degrees >= -67.5f && degrees < -22.5f)   return "Walk Forward Left";

		return "Rifle Aiming Idle";
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
PlayerCharacter::~PlayerCharacter() {
	// 析构逻辑
}

void PlayerCharacter::LoadAssets()
{
	// 如果模型已经存在，直接返回，不再重复加载
	if (g_pSharedPlayerModel) return;

	g_pSharedPlayerModel = new SkinningModel();
	g_pSharedPlayerModel->Load("resource/model/Idle.fbx", 1.0f);

	// 加载所有动作
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
	m_LaserTexID = Texture_LoadFromFile(L"resource/texture/Laser.png");
	Billboard_Initialize();

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

		// C. 开火逻辑
		if (isFiring && m_ShootTimer <= 0.0f && !m_IsReloading && m_CurrentAmmo > 0) {
			const auto& nameMap = m_pModel->GetSkeleton().nameToIndex;
			if (nameMap.count("mixamorig:RightHand")) {
				int handIdx = nameMap.at("mixamorig:RightHand");
				XMMATRIX handMat = m_Animator.GetBoneGlobalMatrix(handIdx);
				XMMATRIX world = XMMatrixScaling(m_Scale, m_Scale, m_Scale) * XMMatrixRotationY(m_RotationY + XM_PI) * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
				XMMATRIX gunLocal = XMMatrixScaling(m_GunScale, m_GunScale, m_GunScale) * XMMatrixRotationRollPitchYaw(m_GunPitch, m_GunYaw, m_GunRoll) * XMMatrixTranslation(m_GunOffset.x, m_GunOffset.y, m_GunOffset.z);
				XMMATRIX gunWorld = gunLocal * handMat * world;

				XMVECTOR muzzleLocalV = XMLoadFloat3(&m_MuzzleLocalOffset);
				XMVECTOR bulletStartPos = XMVector3TransformCoord(muzzleLocalV, gunWorld);
				XMVECTOR bulletDir = gunWorld.r[0];
				float offsetDistance = 0.5f;
				bulletStartPos = XMVectorAdd(bulletStartPos, XMVectorScale(bulletDir, offsetDistance));

				XMFLOAT3 p, v;
				XMStoreFloat3(&p, bulletStartPos);
				XMStoreFloat3(&v, bulletDir);
				Bullet_Create(p, v);
				m_CurrentAmmo--;

				Player_EmitSound(m_Position, 25.0f);
				m_ShootTimer = m_FireRate;
			}
		}
	}

	// --- 5. 动画更新 ---
	m_Animator.Update(dt);
}

void PlayerCharacter::Draw(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj) {
	if (!m_pModel) return;

	// 1. 设置着色器全局矩阵
	SkinningShader_3D_SetViewMatrix(view);
	SkinningShader_3D_SetProjectMatrix(proj);

	// 2. 构造世界矩阵
	DirectX::XMMATRIX world = DirectX::XMMatrixScaling(m_Scale, m_Scale, m_Scale) * DirectX::XMMatrixRotationY(m_RotationY + XM_PI) * DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	SkinningShader_3D_SetWorldMatrix(world);

	// 3. 骨骼传输
	auto bones = m_Animator.GetFinalBoneMatrices(m_pModel->GetSkeleton());
	SkinningShader_3D_SetBoneTransforms(bones);

	// 4. 绘制角色
	SkinningShader_3D_Begin();
	m_pModel->Draw();

	// 5. 绘制武器与激光 (如果非近战状态)
	if (m_MeleeTimer <= 0.2f)
	{
		const auto& nameMap = m_pModel->GetSkeleton().nameToIndex;
		if (nameMap.count("mixamorig:RightHand")) {
			int handIdx = nameMap.at("mixamorig:RightHand");
			XMMATRIX handMat = m_Animator.GetBoneGlobalMatrix(handIdx);

			XMMATRIX gunLocal =
				XMMatrixScaling(m_GunScale, m_GunScale, m_GunScale) *
				XMMatrixRotationRollPitchYaw(m_GunPitch, m_GunYaw, m_GunRoll) *
				XMMatrixTranslation(m_GunOffset.x, m_GunOffset.y, m_GunOffset.z);

			XMMATRIX gunWorld = gunLocal * handMat * world;

			// 画枪
			ModelDraw(m_pGunModel, gunWorld);

			// 画激光瞄准线
			XMVECTOR muzzleLocalV = XMLoadFloat3(&m_MuzzleLocalOffset);
			XMVECTOR laserStartPos = XMVector3TransformCoord(muzzleLocalV, gunWorld);
			XMVECTOR gunForwardDir = XMVector3Normalize(gunWorld.r[0]);
			XMVECTOR laserEndPos = laserStartPos + (gunForwardDir * m_LaserLength);

			Laser_Billboard_Draw(m_LaserTexID, laserStartPos, laserEndPos, 0.02f);
		}
	}
}

void PlayerCharacter::DrawShadow(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj)
{
	if (!m_pModel) return;

	// 1. 绘制玩家 (蒙皮深度)
	SkinningShader_3D_BeginDepthOnly();
	SkinningShader_3D_SetViewMatrix(lightView);
	SkinningShader_3D_SetProjectMatrix(lightProj);

	DirectX::XMMATRIX world = DirectX::XMMatrixScaling(m_Scale, m_Scale, m_Scale) * DirectX::XMMatrixRotationY(m_RotationY + DirectX::XM_PI) * DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	SkinningShader_3D_SetWorldMatrix(world);

	auto bones = m_Animator.GetFinalBoneMatrices(m_pModel->GetSkeleton());
	SkinningShader_3D_SetBoneTransforms(bones);
	m_pModel->Draw();

	// 2. 绘制枪械 (静态阴影)
	if (m_MeleeTimer <= 0.2f && m_pGunModel)
	{
		// 切换回静态阴影 Shader 状态
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

AABB PlayerCharacter::GetAABB() const {
	float halfWidth = 0.5f;
	float height = 1.8f;

	return {
		{ m_Position.x - halfWidth, m_Position.y,          m_Position.z - halfWidth },
		{ m_Position.x + halfWidth, m_Position.y + height, m_Position.z + halfWidth }
	};
}


// ======================================================================================
// 全局 API 包装函数 (C-Style Wrappers)
// ======================================================================================

PlayerCharacter* Player_GetInstance() {
	return g_pPlayerInstance;
}

bool Sound_GetLatest(DirectX::XMFLOAT3& outPos, float& outRadius) {
	if (g_SoundTimer > 0.0f) {
		outPos = g_LastSoundPos;
		outRadius = g_SoundIntensity;
		return true;
	}
	return false;
}

DirectX::XMFLOAT3 Player_GetPosition() {
	if (g_pPlayerInstance) {
		return g_pPlayerInstance->GetPosition();
	}
	return { 0.0f, 0.0f, 0.0f };
}

void Player_SetPosition(const DirectX::XMFLOAT3& pos) {
	if (g_pPlayerInstance) {
		g_pPlayerInstance->SetPosition(pos);
	}
}

void Player_Damage(float damage) {
	if (g_pPlayerInstance) {
		g_pPlayerInstance->ApplyDamage(damage);
	}
}

void Player_AddAmmo(int count) {
	if (g_pPlayerInstance) {
		g_pPlayerInstance->AddAmmo(count);
	}
}

void Player_Heal(float amount) {
	if (g_pPlayerInstance) {
		g_pPlayerInstance->Heal(amount);
	}
}