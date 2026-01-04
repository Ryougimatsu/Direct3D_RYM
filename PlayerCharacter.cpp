#include "PlayerCharacter.h"
#include "SkinningShader.h"
#include "Player_Camera.h"
#include "direct3d.h"
#include "texture.h"
#include "billboard.h"
#include "bullet.h"
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

	m_pModel->LoadAnimation("Running", "resource/model/Running.fbx", 1.0f);

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

	// 计算目标角度
	float targetAngle = atan2f(XMVectorGetX(lookDir), XMVectorGetZ(lookDir));

	// 平滑旋转 (给转弯一点重量感)
	float angleDiff = targetAngle - m_RotationY;
	while (angleDiff < -XM_PI) angleDiff += XM_2PI;
	while (angleDiff > XM_PI) angleDiff -= XM_2PI;
	m_RotationY += angleDiff * 0.15f; // 0.15f 是转向灵敏度

	// --- 2. 输入平滑处理 (Damping) ---
	XMVECTOR inputVec = GetInputVector();
	XMVECTOR currentSmoothed = XMLoadFloat3(&m_CurrentMoveDir);
	XMVECTOR smoothedInput = XMVectorLerp(currentSmoothed, inputVec, 8.0f * (float)dt);
	XMStoreFloat3(&m_CurrentMoveDir, smoothedInput);

	float moveLen = XMVectorGetX(XMVector3Length(smoothedInput));

	bool isFiring = (GetAsyncKeyState(VK_LBUTTON) & 0x8000);

	std::string animToPlay = "Rifle Aiming Idle";
	float crossfadeTime = 0.2f;

	// --- 3. 战术动画状态机 ---
	if (moveLen > 0.05f) {
		// 【移动逻辑】
		XMVECTOR forward = XMVectorSet(sinf(m_RotationY), 0, cosf(m_RotationY), 0);
		XMVECTOR right = XMVectorSet(cosf(m_RotationY), 0, -sinf(m_RotationY), 0);

		float fwdDot = XMVectorGetX(XMVector3Dot(forward, smoothedInput));
		float sideDot = XMVectorGetX(XMVector3Dot(right, smoothedInput));

		float angle = atan2f(sideDot, fwdDot);
		animToPlay = SelectTacticalAnim(angle); // 先根据方向选好步法

		// 如果移动中开火，覆盖为移动射击动作
		if (isFiring) {
			animToPlay = "Firing Rifle";
		}

		m_Animator.SetSpeedScale(moveLen); // 移动时匹配步频
	}
	else {
		// 【静止逻辑】
		m_Animator.SetSpeedScale(1.0f); // 静止时恢复正常动画速率

		if (isFiring) {
		
			animToPlay = "Firing Rifle Idle";
		}
		else {
			animToPlay = "Rifle Aiming Idle";
		}
	}

	// --- 4. 应用物理位移与动画 ---
	XMVECTOR pos = XMLoadFloat3(&m_Position);
	pos += smoothedInput * m_MoveSpeed * (float)dt; // 仅在此处更新一次位置
	XMStoreFloat3(&m_Position, pos);

	m_Animator.PlayAnimation(m_pModel->GetAnimation(animToPlay), true, crossfadeTime);
	m_Animator.Update(dt);


	// --- 5. 开火逻辑 ---
	if (m_ShootTimer > 0.0f) {
		m_ShootTimer -= (float)dt; // 计时器倒计时
	}

	if (isFiring && m_ShootTimer <= 0.0f) {
		// A. 计算当前帧枪口的世界矩阵 (参考 Draw 函数里的逻辑)
		const auto& nameMap = m_pModel->GetSkeleton().nameToIndex;
		if (nameMap.count("mixamorig:RightHand")) {
			int handIdx = nameMap.at("mixamorig:RightHand");
			XMMATRIX handMat = m_Animator.GetBoneGlobalMatrix(handIdx);

			// 构造枪支的世界矩阵
			XMMATRIX world = XMMatrixScaling(m_Scale, m_Scale, m_Scale) * XMMatrixRotationY(m_RotationY + XM_PI) * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

			XMMATRIX gunLocal = XMMatrixScaling(m_GunScale, m_GunScale, m_GunScale) *
				XMMatrixRotationRollPitchYaw(m_GunPitch, m_GunYaw, m_GunRoll) *
				XMMatrixTranslation(m_GunOffset.x, m_GunOffset.y, m_GunOffset.z);

			XMMATRIX gunWorld = gunLocal * handMat * world;

			// B. 获取枪口世界位置和方向
			XMVECTOR muzzleLocalV = XMLoadFloat3(&m_MuzzleLocalOffset);
			XMVECTOR bulletStartPos = XMVector3TransformCoord(muzzleLocalV, gunWorld);

			// 获取枪的朝向 (根据你上一轮调试的结果，可能是 r[2] 或 r[0])
			XMVECTOR bulletDir = gunWorld.r[0];

			float offsetDistance = 0.5f;
			bulletStartPos = XMVectorAdd(bulletStartPos, XMVectorScale(bulletDir, offsetDistance));
			// C. 创建子弹
			XMFLOAT3 pos, vel;
			XMStoreFloat3(&pos, bulletStartPos);
			XMStoreFloat3(&vel, bulletDir);

			Bullet_Create(pos, vel);
			Player_EmitSound(m_Position, 25.0f);
			// D. 重置计时器
			m_ShootTimer = m_FireRate;
		}
	}

	if (g_SoundTimer > 0.0f) g_SoundTimer -= (float)dt;// 声音衰减

	// 更新无敌帧计时器
	if (m_InvincibleTimer > 0.0f) {
		m_InvincibleTimer -= (float)dt;
		if (m_InvincibleTimer < 0.0f) m_InvincibleTimer = 0.0f;
	}

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

