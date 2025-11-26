#pragma once
#include <DirectMath.h>
class Enemy {

protected:
	class State
	{

	public:
		virtual ~State() = default;
		virtual void Update(double elapsed_time) = 0;
		virtual void Draw() const = 0;
	};

private:
	State* m_pState = {};
	State* m_pNextState = {};
public:
	virtual ~Enemy() = default;
	virtual void Update(double elapsed_time);
	virtual void Draw() const;
	void UpdateState();
	virtual bool IsDestroyed() const = 0;
protected:
	void ChangeState(State* pNextState);
};

void Enemy_Initialize();
void Enemy_Finalize();
void Enemy_Update(double elapsed_time);
void Enemy_Draw();
void Enemy_Create(const DirectX::XMFLOAT3& position);