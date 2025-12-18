#pragma once
#include <DirectXMath.h>

enum class EPlayerState {
	Idle,
	Walk,
	Jump
};

class PlayerState
{
public:
	virtual ~PlayerState() = default;
	virtual void Enter() {}
	virtual void Exit() {}

	// 返回下一帧的状态
	virtual EPlayerState Update(double elapsed_time, DirectX::XMFLOAT3& velocity, DirectX::XMFLOAT3& front) = 0;

	// 处理碰撞后的逻辑
	virtual EPlayerState OnCollision(bool isGrounded, DirectX::XMFLOAT3& velocity) {
		return EPlayerState::Idle; // 默认返回值
	}
};

// 具体状态类声明
class StateIdle : public PlayerState {
public:
	void Enter() override;
	EPlayerState Update(double elapsed_time, DirectX::XMFLOAT3& velocity, DirectX::XMFLOAT3& front) override;
};

class StateWalk : public PlayerState {
public:
	void Enter() override;
	EPlayerState Update(double elapsed_time, DirectX::XMFLOAT3& velocity, DirectX::XMFLOAT3& front) override;
};

class StateAir : public PlayerState {
public:
	void Enter() override;
	EPlayerState Update(double elapsed_time, DirectX::XMFLOAT3& velocity, DirectX::XMFLOAT3& front) override;
	EPlayerState OnCollision(bool isGrounded, DirectX::XMFLOAT3& velocity) override;
};