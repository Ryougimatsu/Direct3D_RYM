#pragma once
#include <DirectXMath.h>
#include <vector>

// ============================================================
// 单个粒子的数据结构
// ============================================================
struct Particle {
	DirectX::XMFLOAT3 Position;  // 世界空间位置
	DirectX::XMFLOAT3 Velocity;  // 速度向量 (方向 * 速率)
	DirectX::XMFLOAT4 Color;     // RGBA 颜色 (w 分量为 Alpha 透明度)
	float Age;                   // 当前已存活时间 (秒)
	float LifeTime;              // 最大生存期 (秒), 超过后自动销毁
	float Size;                  // 公告板缩放大小
	bool Active;                 // 是否处于激活状态
	float Gravity;               // 该粒子受到的重力加速度 (仅影响 Y 轴)
};

// ============================================================
// 粒子系统类
// 管理粒子池的生命周期、物理更新和渲染
// ============================================================
class ParticleSystem {
private:
	std::vector<Particle> m_Particles; // 粒子对象池
	int m_MaxParticles;                // 池的最大容量
	int m_TextureID;                   // 所有粒子共用的纹理 ID

public:
	// --- 生命周期 ---
	void Initialize(int maxParticles, int texID); // 初始化粒子池并设置纹理
	void Finalize();                              // 清空粒子池

	// --- 每帧更新与绘制 ---
	void Update(double dt);                       // 更新所有激活粒子的物理状态
	void Draw();                                  // 使用公告板绘制所有激活粒子

	// --- 各类型粒子发射器 ---
	void Emit(DirectX::XMFLOAT3 pos, int count);                                  // 通用爆炸粒子 (橙色, 向四周扩散)
	void EmitSmoke(DirectX::XMFLOAT3 pos, int count);                             // 烟雾粒子 (灰色, 缓慢上升)
	void EmitMuzzleFlash(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count); // 枪口闪光粒子 (白色, 静止, 极短寿命)
	void EmitMuzzleFire(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count);  // 枪口火焰粒子 (黄色, 沿枪管喷射)
	void EmitBlood(DirectX::XMFLOAT3 pos, int count);                             // 血液粒子 (暗红, 受重力影响抛物下落)
};
