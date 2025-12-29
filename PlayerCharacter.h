#pragma once
#include "SkinningModel.h"
#include "Animator.h"
#include <DirectXMath.h>
#include "model.h"

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
	DirectX::XMFLOAT3 GetPosition() const { return m_Position; }



private:
	// 资源与组件
	SkinningModel* m_pModel = nullptr;
	Animator       m_Animator;
	MODEL* m_pGunModel;
	// 状态机变量
	CharacterState m_CurrentState = CharacterState::Idle;
	float          m_StateTimer = 0.0f; // 用于控制测试流程的计时器

	// 空间属性
	DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
	float m_RotationY = 0.0f;
	float m_Scale = 0.01f;     // 修正未识别的关键：定义缩放系数
	float m_MoveSpeed = 2.0f;
	float m_GunScale = 1.0f;
};