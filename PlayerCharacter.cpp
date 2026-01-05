#include "PlayerCharacter.h"
#include "SkinningShader.h"
#include "Player_Camera.h"
#include "direct3d.h"
#include "texture.h"
#include "billboard.h"
#include "bullet.h"
#include "enemy.h"
using namespace DirectX;

PlayerCharacter* g_pPlayerInstance = nullptr;
namespace {
	float m_GunPitch = DirectX::XMConvertToRadians(5.0f);  // 绕 X
	float m_GunYaw = DirectX::XMConvertToRadians(-90.0f);                                 // 绕 Y
	float m_GunRoll = DirectX::XMConvertToRadians(90.0f);  // 绕 Z
	DirectX::XMFLOAT3 m_GunOffset = { 0.05f, 0.05f, 0.0f };  // 手心里的偏移
	DirectX::XMFLOAT3 m_MuzzleLocalOffset = { 0.0f, 0.0f, 0.5f };
	float m_LaserLength = 20.0f;
	int m_LaserTexID = -1;

	DirectX::XMVECTOR GetInputVector() {
		using namespace DirectX;
		XMVECTOR input = XMVectorSet(0, 0, 0, 0);

		// 检测键盘状态
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

	//声音
	DirectX::XMFLOAT3 g_LastSoundPos = { 0, 0, 0 }; // 声音位置
	float g_SoundIntensity = 0.0f;               // 声音强度（半径）
	float g_SoundTimer = 0.0f;                    // 声音持续时间（秒）
}


PlayerCharacter* Player_GetInstance() {
	return g_pPlayerInstance;
}

void Player_EmitSound(const DirectX::XMFLOAT3& pos, float radius) {
	g_LastSoundPos = pos;
	g_SoundIntensity = radius;
	g_SoundTimer = 0.5f; // 声音在空气中存在 0.5 秒
}

bool Sound_GetLatest(DirectX::XMFLOAT3& outPos, float& outRadius) {
	if (g_SoundTimer > 0.0f) {
		outPos = g_LastSoundPos;
		outRadius = g_SoundIntensity;
		return true;
	}
	return false;
}

void Player_SetPosition(const DirectX::XMFLOAT3& pos)
{
	if (g_pPlayerInstance) {
		g_pPlayerInstance->SetPosition(pos);
	}
}

bool PlayerCharacter::Initialize() {

	g_pPlayerInstance = this;

	m_pModel = new SkinningModel();

	if (!m_pModel->Load("resource/model/Idle.fbx", 1.0f)) return false;
	m_pModel->LoadAnimation("Walk Forward", "resource/model/Character_Model/Walk Forward.fbx", 1.0f);
	m_pModel->LoadAnimation("Walk Backward", "resource/model/Character_Model/Walk Backward.fbx", 1.0f);
	m_pModel->LoadAnimation("Walk Left", "resource/model/Character_Model/Walk Left.fbx", 1.0f);
	m_pModel->LoadAnimation("Walk Right", "resource/model/Character_Model/Walk Right.fbx", 1.0f);
	m_pModel->LoadAnimation("Walk Forward Right", "resource/model/Character_Model/Walk Forward Right.fbx", 1.0f);
	m_pModel->LoadAnimation("Walk Forward Left", "resource/model/Character_Model/Walk Forward Left.fbx", 1.0f);
	m_pModel->LoadAnimation("Walk Backward Right", "resource/model/Character_Model/Walk Backward Right.fbx", 1.0f);
	m_pModel->LoadAnimation("Walk Backward Left", "resource/model/Character_Model/Walk Backward Left.fbx", 1.0f);
	m_pModel->LoadAnimation("Firing Rifle", "resource/model/Character_Model/Firing Rifle.fbx", 1.0f);
	m_pModel->LoadAnimation("Firing Rifle Idle", "resource/model/Character_Model/Firing Rifle idle.fbx", 1.0f);

	m_pModel->LoadAnimation("Slash Advance", "resource/model/Character_Model/Slash Advance.fbx", 1.0f);

	m_pGunModel = ModelLoad("resource/model/M4a4.fbx", 1.0f);

	m_Animator.PlayAnimation(m_pModel->GetDefaultAnimation(), true);
	m_LaserTexID = Texture_LoadFromFile(L"resource/texture/Laser.png");
	Billboard_Initialize();
	return true;
}

std::string SelectTacticalAnim(float angle) {
	using namespace DirectX;

	// 将角度转换为度数，方便调试和理解 (范围: -180 到 180)
	float degrees = XMConvertToDegrees(angle);

	// 1. Forward (正前): -22.5 到 22.5
	if (degrees >= -22.5f && degrees < 22.5f) return "Walk Forward";

	// 2. Forward Right (右前): 22.5 到 67.5
	if (degrees >= 22.5f && degrees < 67.5f) return "Walk Forward Right";

	// 3. Right (正右): 67.5 到 112.5
	if (degrees >= 67.5f && degrees < 112.5f) return "Walk Right";

	// 4. Backward Right (右后): 112.5 到 157.5
	if (degrees >= 112.5f && degrees < 157.5f) return "Walk Backward Right";

	// 5. Backward (正后): 157.5 到 180 或 -180 到 -157.5
	if (degrees >= 157.5f || degrees < -157.5f) return "Walk Backward";

	// 6. Backward Left (左后): -157.5 到 -112.5
	if (degrees >= -157.5f && degrees < -112.5f) return "Walk Backward Left";

	// 7. Left (正左): -112.5 到 -67.5
	if (degrees >= -112.5f && degrees < -67.5f) return "Walk Left";

	// 8. Forward Left (左前): -67.5 到 -22.5
	if (degrees >= -67.5f && degrees < -22.5f) return "Walk Forward Left";

	return "Rifle Aiming Idle";
}



void PlayerCharacter::Update(double dt) {
	// 获取相机矩阵用于计算鼠标位置
	XMMATRIX view = XMLoadFloat4x4(&Player_Camera_GetViewMatrix());
	XMMATRIX proj = XMLoadFloat4x4(&Player_Camera_GetProjectionMatrix());

	// --- 1. 旋转逻辑：始终指向鼠标 ---
	XMVECTOR mousePos = GetMouseWorldPos(view, proj);
	XMVECTOR playerPos = XMLoadFloat3(&m_Position);
	XMVECTOR lookDir = XMVectorSubtract(mousePos, playerPos);

	float targetAngle = atan2f(XMVectorGetX(lookDir), XMVectorGetZ(lookDir));
	float angleDiff = targetAngle - m_RotationY;
	while (angleDiff < -XM_PI) angleDiff += XM_2PI;
	while (angleDiff > XM_PI) angleDiff -= XM_2PI;
	m_RotationY += angleDiff * 0.15f;

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

	// =========================================================
	// 3. 近战触发逻辑 (优先级最高)
	// =========================================================
	if ((GetAsyncKeyState('F') & 0x8000) && m_MeleeTimer <= 0.0f) {
		XMVECTOR forward = XMVectorSet(sinf(m_RotationY), 0, cosf(m_RotationY), 0);

		// 1. 造成伤害
		Enemy_ApplyMeleeDamage(m_Position, forward, 2.5f, 120.0f);

		// 2. 播放攻击动画 (关键：false 表示不循环，只播一次)
		// 确保你在 Initialize() 里 LoadAnimation 加载了 "Slash Advance"
		m_Animator.PlayAnimation(m_pModel->GetAnimation("Slash Advance"), false, 0.02f);

		// 3. 设置硬直时间
		// 假设动作长 1.2 秒，设置 1.5 秒冷却，留 0.3 秒后摇
		m_MeleeTimer = 2.0f;
	}

	// =========================================================
	// 4. 动画状态机 & 移动逻辑
	// =========================================================

	// [动作锁]：如果 Timer > 0.3f，说明动作正在播放中，禁止切回站立或走路
	bool isInMeleeAnimation = (m_MeleeTimer > 0.2f);

	if (!isInMeleeAnimation)
	{
		// >>> 只要不是在攻击硬直中，就会进入这里 <<<

		std::string animToPlay = "Rifle Aiming Idle"; // 默认目标：持枪站立
		float crossfadeTime = 0.2f;

		// --- A. 移动检测 ---
		if (moveLen > 0.05f) {
			// [正在移动]
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
			// [移动结束 / 站立不动]
			// 这里的逻辑保证了：
			// 1. 刚打完架(硬直结束) -> 进到这里 -> 设为 Idle
			// 2. 刚跑完步(松开键盘) -> 进到这里 -> 设为 Idle
			if (isFiring) animToPlay = "Firing Rifle Idle";
			else animToPlay = "Rifle Aiming Idle";

			m_Animator.SetSpeedScale(1.0f);
		}

		// [应用动画] (true = 循环播放)
		// 这行代码会平滑过渡到上面选定的动作 (比如从 Slash 过渡回 Idle)
		m_Animator.PlayAnimation(m_pModel->GetAnimation(animToPlay), true, crossfadeTime);

		// --- B. 物理位移 ---
		// 只有没在硬直时，才允许位移
		XMVECTOR pos = XMLoadFloat3(&m_Position);
		pos += smoothedInput * m_MoveSpeed * (float)dt;
		XMStoreFloat3(&m_Position, pos);

		// --- C. 开火逻辑 ---
		if (isFiring && m_ShootTimer <= 0.0f) {
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

				Player_EmitSound(m_Position, 25.0f);
				m_ShootTimer = m_FireRate;
			}
		}
	}

	// --- 5. 动画更新 (必须在所有逻辑之后) ---
	m_Animator.Update(dt);
}

void PlayerCharacter::Draw(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj) {
	if (!m_pModel) return;

	// 1. 设置着色器全局矩阵
	SkinningShader_3D_SetViewMatrix(view);
	SkinningShader_3D_SetProjectMatrix(proj);

	// 2. 构造世界矩阵 (这里缩小了模型并设置位置)
	DirectX::XMMATRIX world = DirectX::XMMatrixScaling(m_Scale, m_Scale, m_Scale) * DirectX::XMMatrixRotationY(m_RotationY + XM_PI) * DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	SkinningShader_3D_SetWorldMatrix(world);

	// 3. 获取并传输当前的骨骼矩阵调色板
	auto bones = m_Animator.GetFinalBoneMatrices(m_pModel->GetSkeleton());
	SkinningShader_3D_SetBoneTransforms(bones);

	// 4. 执行渲染
	SkinningShader_3D_Begin();
	m_pModel->Draw();
	const auto& nameMap = m_pModel->GetSkeleton().nameToIndex;
	if (nameMap.count("mixamorig:RightHand")) {
		int handIdx = nameMap.at("mixamorig:RightHand");

		XMMATRIX handMat = m_Animator.GetBoneGlobalMatrix(handIdx);

		// 只做缩放 + 旋转
		XMMATRIX gunLocal =
			XMMatrixScaling(m_GunScale, m_GunScale, m_GunScale) *
			XMMatrixRotationRollPitchYaw(m_GunPitch, m_GunYaw, m_GunRoll) *
			XMMatrixTranslation(m_GunOffset.x, m_GunOffset.y, m_GunOffset.z);

		XMMATRIX gunWorld = gunLocal * handMat * world;

		ModelDraw(m_pGunModel, gunWorld);


		// 绘制激光瞄准线
		XMVECTOR muzzleLocalV = XMLoadFloat3(&m_MuzzleLocalOffset);
		XMVECTOR laserStartPos = XMVector3TransformCoord(muzzleLocalV, gunWorld);

		// B. 提取世界空间中的枪口前方方向 (Forward Direction)
		XMVECTOR gunForwardDir = gunWorld.r[0];

		gunForwardDir = XMVector3Normalize(gunForwardDir);

		// C. 计算红线终点位置 (End Pos)
		// 终点 = 起点 + (方向向量 * 长度)
		XMVECTOR laserEndPos = laserStartPos + (gunForwardDir * m_LaserLength);

		// D. 执行绘制
		Laser_Billboard_Draw(m_LaserTexID, laserStartPos, laserEndPos, 0.02f); // 红色，带一点透明
	}
}

void PlayerCharacter::ApplyDamage(float damage)
{
	if (m_InvincibleTimer > 0.0f) {
		return;
	}
	m_HP -= damage;

	m_InvincibleTimer = m_InvincibleDuration;
}

PlayerCharacter::~PlayerCharacter() {
	if (m_pModel) {
		m_pModel->Release();
		delete m_pModel;
	}
	if (m_pGunModel) {
		ModelRelease(m_pGunModel);
		m_pGunModel = nullptr;
	}
}


DirectX::XMFLOAT3 Player_GetPosition() {
	if (g_pPlayerInstance) {
		return g_pPlayerInstance->GetPosition(); // 调用类成员
	}
	return { 0.0f, 0.0f, 0.0f };
}

void Player_Damage(float damage) {
	if (g_pPlayerInstance) {
		g_pPlayerInstance->ApplyDamage(damage);
	}
}

