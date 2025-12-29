#include "PlayerCharacter.h"
#include "SkinningShader.h"
using namespace DirectX;

namespace {

	float m_GunPitch = DirectX::XMConvertToRadians(0.0f);  // 绕 X
	float m_GunYaw = DirectX::XMConvertToRadians(-90.0f);                                 // 绕 Y
	float m_GunRoll = DirectX::XMConvertToRadians(90.0f);  // 绕 Z

	DirectX::XMFLOAT3 m_GunOffset = { -0.01f, 0.19f, -0.02f };  // 手心里的偏移
}
bool PlayerCharacter::Initialize() {
	m_pModel = new SkinningModel();

	if (!m_pModel->Load("resource/model/Idle.fbx", 1.0f)) return false;


	m_pModel->LoadAnimation("Running", "resource/model/Running.fbx", 1.0f);


	m_Animator.PlayAnimation(m_pModel->GetDefaultAnimation(), true);

	m_pGunModel = ModelLoad("resource/model/M4a4.fbx", 1.0f);
	return true;
}

void PlayerCharacter::Update(double dt) {
	m_StateTimer += (float)dt;

	// --- 简单的状态机逻辑控制测试流程 ---
	// 0s - 3s: Idle (待机)
	// 3s - 8s: Running (向前跑)
	// 8s 以后: Idle (停下)

	CharacterState nextState = m_CurrentState;

	if (m_StateTimer < 3.0f) {
		nextState = CharacterState::Idle;
	}
	else if (m_StateTimer < 8.0f) {
		nextState = CharacterState::Running;
		// 移动逻辑：处于 Running 状态时改变坐标
		m_Position.z += m_MoveSpeed * (float)dt;
	}
	else {
		nextState = CharacterState::Idle;
	}

	// 检测状态切换并应用新动画
	if (nextState != m_CurrentState) {
		m_CurrentState = nextState;
		if (m_CurrentState == CharacterState::Running) {
			m_Animator.PlayAnimation(m_pModel->GetAnimation("Running"), true,0.5f);
		}
		else {
			m_Animator.PlayAnimation(m_pModel->GetDefaultAnimation(), true,0.5f);
		}
	}

	// 驱动骨骼矩阵计算
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
		XMMATRIX gunLocalNoOffset =
			XMMatrixScaling(m_GunScale, m_GunScale, m_GunScale) *
			XMMatrixRotationRollPitchYaw(m_GunPitch, m_GunYaw, m_GunRoll);

		// 挂到手上，再在世界空间平移
		XMMATRIX gunWorld =
			gunLocalNoOffset *
			handMat *
			world *
			XMMatrixTranslation(m_GunOffset.x, m_GunOffset.y, m_GunOffset.z);

		ModelDraw(m_pGunModel, gunWorld);
	}
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