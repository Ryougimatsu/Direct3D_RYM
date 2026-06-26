#include "SkillSystem.h"

#include <algorithm>

namespace
{
	constexpr int TEMPORARY_INFINITE_AMMO_SECONDS = 10;
	constexpr int MAX_HP_BONUS_AMOUNT = 20;
	constexpr int BULLET_PIERCE_BONUS = 1;
	constexpr int MAX_BULLET_PIERCE = 5;
	constexpr int BULLET_DAMAGE_BONUS_AMOUNT = 5;
	constexpr float MOVE_SPEED_BONUS_AMOUNT = 0.2f;
	constexpr float MAX_MOVE_SPEED = 500.0f;

	constexpr int INFINITE_AMMO_MAX_STACK = 1;
	constexpr int MAX_HP_MAX_STACK = 10;
	constexpr int BULLET_PIERCE_MAX_STACK = MAX_BULLET_PIERCE;
	constexpr int BULLET_DAMAGE_MAX_STACK = 20;
	constexpr int MOVE_SPEED_MAX_STACK = 10;

	float ResolveFloatValue(const SkillDefinition& skill)
	{
		if (skill.floatValue != 0.0f)
		{
			return skill.floatValue;
		}
		return static_cast<float>(skill.intValue);
	}
}

SkillSystem::SkillSystem()
	: m_RandomEngine(std::random_device{}())
{
}

void SkillSystem::Initialize()
{
	m_SkillPool.clear();

	m_SkillPool.push_back({
		SkillType::TemporaryInfiniteAmmo,
		L"Infinite Ammo",
		L"For 10 seconds, shooting does not consume ammo.",
		TEMPORARY_INFINITE_AMMO_SECONDS,
		static_cast<float>(TEMPORARY_INFINITE_AMMO_SECONDS),
		INFINITE_AMMO_MAX_STACK
	});

	m_SkillPool.push_back({
		SkillType::IncreaseMaxHp,
		L"Vitality Boost",
		L"Max HP +20 and heal 20 HP.",
		MAX_HP_BONUS_AMOUNT,
		static_cast<float>(MAX_HP_BONUS_AMOUNT),
		MAX_HP_MAX_STACK
	});

	m_SkillPool.push_back({
		SkillType::IncreaseBulletPierce,
		L"Piercing Rounds",
		L"Bullets pierce 1 additional target.",
		BULLET_PIERCE_BONUS,
		static_cast<float>(BULLET_PIERCE_BONUS),
		BULLET_PIERCE_MAX_STACK
	});

	m_SkillPool.push_back({
		SkillType::IncreaseBulletDamage,
		L"Firepower Up",
		L"Bullet damage +5.",
		BULLET_DAMAGE_BONUS_AMOUNT,
		static_cast<float>(BULLET_DAMAGE_BONUS_AMOUNT),
		BULLET_DAMAGE_MAX_STACK
	});

	m_SkillPool.push_back({
		SkillType::IncreaseMoveSpeed,
		L"Move Speed Up",
		L"Move speed +0.2.",
		0,
		MOVE_SPEED_BONUS_AMOUNT,
		MOVE_SPEED_MAX_STACK
	});
}

std::vector<SkillDefinition> SkillSystem::DrawRandomSkills(std::size_t count)
{
	if (m_SkillPool.empty() || count == 0)
	{
		return {};
	}

	std::vector<SkillDefinition> choices = m_SkillPool;
	std::shuffle(
		choices.begin(),
		choices.end(),
		m_RandomEngine);

	if (choices.size() > count)
	{
		choices.resize(count);
	}
	return choices;
}

void SkillSystem::ApplySkill(
	const SkillDefinition& skill,
	PlayerStats& stats) const
{
	switch (skill.type)
	{
	case SkillType::TemporaryInfiniteAmmo:
		stats.infiniteAmmo = true;
		// Refresh duration instead of stacking multiple pickups.
		stats.infiniteAmmoTimer = ResolveFloatValue(skill);
		break;

	case SkillType::IncreaseMaxHp:
	{
		const float amount = ResolveFloatValue(skill);
		stats.maxHp += amount;
		stats.currentHp += amount;
		stats.currentHp = std::min(stats.currentHp, stats.maxHp);
		break;
	}

	case SkillType::IncreaseBulletPierce:
		stats.bulletPierce = std::clamp(
			stats.bulletPierce + std::max(1, skill.intValue),
			0,
			MAX_BULLET_PIERCE);
		break;

	case SkillType::IncreaseBulletDamage:
		stats.bulletDamage += ResolveFloatValue(skill);
		break;

	case SkillType::IncreaseMoveSpeed:
		stats.moveSpeed = std::clamp(
			stats.moveSpeed + ResolveFloatValue(skill),
			0.0f,
			MAX_MOVE_SPEED);
		break;

	default:
		break;
	}

	stats.ClampCurrentHp();
}
