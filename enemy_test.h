#pragma once
#include "enemy.h"
#include <DirectXMath.h>
#include "model.h"
#include "Animator.h"
#include "SkinningModel.h"
#include "Player_Camera.h"


class EnemyTest : public Enemy {
private:
    // 子类独有的数据
    DirectX::XMFLOAT3 m_position{};
    DirectX::XMFLOAT3 m_Rotation = { 0.0f, 0.0f, 0.0f }; // [必须有]

    float m_DetectionAngle = 5.0f;
    float m_HP = 100.0f;
	float m_DetectionRadius = 8.0f;  // 探测半径
	float m_AttackRadius = 1.2f;     // 攻击半径
	float m_AttackCooldown = 1.0f;   // 攻击间隔（秒）
	double m_LastAttackTimer = 0.0;  // 攻击计时器
	float m_FOVAngle = 90.0f;        // 视野总角度（度）
    bool m_bAlertedStatus = false; //   状态标记
	bool m_bIsDestroyed = false;   // 是否已被销毁


	Animator m_Animator;            // 每个敌人私有的动画器
	static const Animation* g_pIdleAnim;  // 全局共享的 Idle 动画资源
    static const Animation* g_pWalkAnim;
    static const Animation* g_pAttackAnim;
    static const Animation* g_pScreamAnim;



public:
    EnemyTest(const DirectX::XMFLOAT3& position);
    ~EnemyTest() override;


    const DirectX::XMFLOAT3& GetPosition() const override { return m_position; }
    void SetPosition(const DirectX::XMFLOAT3& pos) override { m_position = pos; }
    void Damage(float damage) override { m_HP -= damage; }
    bool IsDestroyed() const override { return m_HP <= 0.0f; }
    AABB GetAABB() override;

    DirectX::XMFLOAT3 GetRotation() const override { return m_Rotation; }

    bool IsAlerted() const override { return m_bAlertedStatus; }
    void ChangeState(State* pNextState) override;
    void ApplyKnockback(const DirectX::XMVECTOR& direction, float force) override;
    void SetAlerted(bool alerted);

    void SetRotationY(float angle) { m_Rotation.y = angle; }

    void Update(double elapsed_time) override;
    void Draw(DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj) const override;

    static void LoadAssets();
    static void UnloadAssets();

private:
    // 状态类声明
    class EnemyTest_StatePatrol : public State {
    private:
        EnemyTest* m_pOwner = {};
        float m_PointX = {};
        double m_AccumulatedTime = {};

		DirectX::XMFLOAT3 m_PatrolOrigin; // 巡逻中心（通常是敌人的初始位置）
		DirectX::XMFLOAT3 m_TargetPoint;  // 当前随机选中的目标点
		float m_WaitTimer = 0.0f;       // 当前已等待时间
		const float WAIT_DURATION = 2.0f; // 停顿观察的总时长（秒）
        const float MAX_WANDER_RADIUS = 8.0f; // 随机巡逻的最大半径
        bool m_bAlerted = false;

        void PickRandomTarget();

    public:
        EnemyTest_StatePatrol(EnemyTest* pOwner);
        void Update(double elapsed_time) override;
        void Draw() const override;
    };

    class EnemyTest_StateChase : public State {
    private:
        EnemyTest* m_pOwner = {};
        double m_AccumulatedTime = {};
        bool m_HasDealtDamageInThisCycle = false;
    public:
        EnemyTest_StateChase(EnemyTest* pOwner);
        void Update(double elapsed_time) override;
        void Draw() const override;
    };
};