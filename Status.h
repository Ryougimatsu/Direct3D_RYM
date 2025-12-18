#pragma once
#include <string>
#include <vector>
#include <algorithm>

// Buff 类型枚举
enum class EBuffType {
	None,
	HealOverTime,   // 持续回血
	Poison,         // 中毒 (持续扣血)
	SpeedUp,        // 加速
	Stun,           // 眩晕 (无法移动)
	DefenseUp       // 防御提升
};

// 单个 Buff 的定义
struct Buff {
	EBuffType type;
	float duration;     // 剩余时间 (秒)
	float value;        // 强度 (例如: 每秒扣多少血，或者速度倍率)
	std::wstring name;  // Buff 名字 (用于显示)

	Buff(EBuffType t, float d, float v, const std::wstring& n)
		: type(t), duration(d), value(v), name(n) {
	}
};