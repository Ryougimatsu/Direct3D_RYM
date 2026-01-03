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

	Animator m_Animator;            // 每个敌人私有的动画器
	static const Animation* g_pIdleAnim;  // 全局共享的 Idle 动画资源
    static const Animation* g_pWalkAnim;
    static const Animation* g_pAttackAnim;



public:
    EnemyTest(const DirectX::XMFLOAT3& position);
    ~EnemyTest() override;


    const DirectX::XMFLOAT3& GetPosition() const override { return m_position; }
    void Damage(float damage) override { m_HP -= damage; }
    bool IsDestroyed() const override { return m_HP <= 0.0f; }
    AABB GetAABB() override;


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