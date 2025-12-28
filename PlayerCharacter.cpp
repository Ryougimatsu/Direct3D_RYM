#include "PlayerCharacter.h"
#include "SkinningShader.h"

bool PlayerCharacter::Initialize() {
	m_pModel = new SkinningModel();

	// 1. 加载带网格的基础模型（假设 Idle.fbx 带有身体模型）
	if (!m_pModel->Load("resource/model/Idle.fbx", 1.0f)) return false;

	// 2. 注入跑步动作（来自另一个只有骨骼动画的 FBX）
	m_pModel->LoadAnimation("Running", "resource/model/Running.fbx", 1.0f);

	// 3. 初始状态设置：播放 Idle 动作
	// 注意：Load 内部默认将第一个动画存为 "Default"，这里我们可以直接按名字取
	m_Animator.PlayAnimation(m_pModel->GetDefaultAnimation(), true);

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
	DirectX::XMMATRIX world = DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f) * DirectX::XMMatrixRotationY(DirectX::XM_PI) * DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	SkinningShader_3D_SetWorldMatrix(world);

	// 3. 获取并传输当前的骨骼矩阵调色板
	auto bones = m_Animator.GetFinalBoneMatrices(m_pModel->GetSkeleton());
	SkinningShader_3D_SetBoneTransforms(bones);

	// 4. 执行渲染
	SkinningShader_3D_Begin();
	m_pModel->Draw();
}

PlayerCharacter::~PlayerCharacter() {
	if (m_pModel) {
		m_pModel->Release();
		delete m_pModel;
	}
}