#pragma once
#include "Status.h"
#include <DirectXMath.h>

class Character {
protected:
	// --- 基础属性 ---
	std::wstring m_Name;
	float m_MaxHP;
	float m_CurrentHP;
	float m_BaseSpeed;    // 基础移动速度

	// --- 状态列表 ---
	std::vector<Buff> m_Buffs;

	// --- 标记 ---
	bool m_IsDead = false;

public:
	Character(const std::wstring& name, float maxHp)
		: m_Name(name), m_MaxHP(maxHp), m_CurrentHP(maxHp), m_BaseSpeed(1.0f) {
	}

	virtual ~Character() = default;

	// --- 核心逻辑 ---

	// 每帧调用：更新 Buff 时间，处理持续掉血等
	virtual void UpdateStatus(double elapsed_time) {
		if (m_IsDead) return;

		// 遍历 Buff 列表
		for (auto it = m_Buffs.begin(); it != m_Buffs.end(); ) {
			// 1. 减少持续时间
			it->duration -= static_cast<float>(elapsed_time);

			// 2. 处理持续效果 (DOT)
			if (it->type == EBuffType::Poison) {
				// 假设 value 是每秒扣血量
				Damage(it->value * static_cast<float>(elapsed_time));
			}
			else if (it->type == EBuffType::HealOverTime) {
				Heal(it->value * static_cast<float>(elapsed_time));
			}

			// 3. 移除过期 Buff
			if (it->duration <= 0.0f) {
				it = m_Buffs.erase(it);
			}
			else {
				++it;
			}
		}
	}

	// --- 战斗接口 ---

	virtual void Damage(float amount) {
		// 可以在这里计算防御力 Buff
		float finalDamage = amount;
		if (HasBuff(EBuffType::DefenseUp)) {
			finalDamage *= 0.5f; // 防御 Buff 减半伤害
		}

		m_CurrentHP -= finalDamage;
		if (m_CurrentHP <= 0.0f) {
			m_CurrentHP = 0.0f;
			m_IsDead = true;
			OnDeath();
		}
	}

	virtual void Heal(float amount) {
		if (m_IsDead) return;
		m_CurrentHP += amount;
		if (m_CurrentHP > m_MaxHP) m_CurrentHP = m_MaxHP;
	}

	virtual void OnDeath() {
		// 子类可以重写，播放死亡动画等
	}

	// --- Buff 操作 ---

	void AddBuff(EBuffType type, float duration, float value, const std::wstring& name) {
		m_Buffs.emplace_back(type, duration, value, name);
	}

	bool HasBuff(EBuffType type) const {
		for (const auto& b : m_Buffs) {
			if (b.type == type) return true;
		}
		return false;
	}

	// 获取当前速度倍率 (考虑 Buff)
	float GetSpeedMultiplier() const {
		float multiplier = 1.0f;
		for (const auto& b : m_Buffs) {
			if (b.type == EBuffType::SpeedUp) multiplier *= b.value; // 例如 value=1.5
			if (b.type == EBuffType::Stun) return 0.0f; // 眩晕无法移动
		}
		return multiplier;
	}

	// --- Getters ---
	float GetHP() const { return m_CurrentHP; }
	float GetMaxHP() const { return m_MaxHP; }
	bool IsDead() const { return m_IsDead; }
	const std::wstring& GetName() const { return m_Name; }
};