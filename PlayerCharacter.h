#pragma once
#include "SkinningModel.h"
#include "Animator.h"
#include <DirectXMath.h>

// 定义状态机状态
enum class CharacterState {
	Idle,
	Running
};

class PlayerCharacter {
public:
	PlayerCharacter() = default;
	~PlayerCharacter();

	bool Initialize();
	void Update(double dt);
	void Draw(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);

private:
	// 资源与组件
	SkinningModel* m_pModel = nullptr;
	Animator       m_Animator;

	// 状态机变量
	CharacterState m_CurrentState = CharacterState::Idle;
	float          m_StateTimer = 0.0f; // 用于控制测试流程的计时器

	// 空间属性
	DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
	float m_MoveSpeed = 2.0f;
};