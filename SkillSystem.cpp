#include "SkillSystem.h"

#include <algorithm>
#include <array>
#include <set>

namespace
{
	constexpr int TEMPORARY_INFINITE_AMMO_SECONDS = 10;
	constexpr int LEGENDARY_INFINITE_AMMO_SECONDS = 20;
	constexpr int MAX_HP_BONUS_AMOUNT = 20;
	constexpr int BULLET_PIERCE_BONUS = 1;
	constexpr int MAX_BULLET_PIERCE = 5;
	constexpr int COMMON_BULLET_DAMAGE_BONUS = 5;
	constexpr int EPIC_BULLET_DAMAGE_BONUS = 12;
	constexpr float MOVE_SPEED_BONUS_AMOUNT = 0.2f;
	constexpr float MAX_MOVE_SPEED = 500.0f;

	constexpr int COMMON_MAGAZINE_BONUS = 5;
	constexpr int EPIC_MAGAZINE_BONUS = 15;
	constexpr float RARE_DROP_RATE_BONUS = 0.10f;
	constexpr float EPIC_DROP_RATE_BONUS = 0.20f;
	constexpr float MAX_ITEM_DROP_RATE_BONUS = 0.50f;

	constexpr int UNLIMITED_STACKS = -1;
	constexpr int BULLET_PIERCE_MAX_STACK = MAX_BULLET_PIERCE;
	constexpr int COMMON_MAGAZINE_MAX_STACK = 10;
	constexpr int EPIC_MAGAZINE_MAX_STACK = 5;
	constexpr int RARE_DROP_RATE_MAX_STACK = 5;
	constexpr int EPIC_DROP_RATE_MAX_STACK = 3;

	struct RarityWeight
	{
		SkillRarity rarity;
		int weight;
	};

	constexpr std::array<RarityWeight, 4> RARITY_WEIGHTS{ {
		{ SkillRarity::Common, 70 },
		{ SkillRarity::Rare, 22 },
		{ SkillRarity::Epic, 7 },
		{ SkillRarity::Legendary, 1 },
	} };

	float ResolveFloatValue(const SkillDefinition& skill)
	{
		if (skill.floatValue != 0.0f)
		{
			return skill.floatValue;
		}
		return static_cast<float>(skill.intValue);
	}

	std::wstring BuildSkillStackKey(const SkillDefinition& skill)
	{
		return
			std::to_wstring(static_cast<int>(skill.type)) +
			L"|" +
			std::to_wstring(static_cast<int>(skill.rarity)) +
			L"|" +
			skill.name +
			L"|" +
			std::to_wstring(skill.intValue) +
			L"|" +
			std::to_wstring(static_cast<int>(skill.floatValue * 1000.0f));
	}
}

SkillSystem::SkillSystem()
	: m_RandomEngine(std::random_device{}())
{
}

void SkillSystem::Initialize()
{
	m_SkillPool.clear();
	m_LastDrawnSkills.clear();
	m_SkillStacks.clear();

	m_SkillPool.push_back({
		SkillType::IncreaseMaxHp,
		L"Vitality Boost",
		L"Max HP +20 and heal 20 HP.",
		MAX_HP_BONUS_AMOUNT,
		0.0f,
		UNLIMITED_STACKS,
		SkillRarity::Common,
		100
	});

	m_SkillPool.push_back({
		SkillType::IncreaseBulletDamage,
		L"Firepower Up",
		L"Bullet damage +5.",
		COMMON_BULLET_DAMAGE_BONUS,
		0.0f,
		UNLIMITED_STACKS,
		SkillRarity::Common,
		100
	});

	m_SkillPool.push_back({
		SkillType::IncreaseMoveSpeed,
		L"Move Speed Up",
		L"移动速度提高 0.2",
		0,
		MOVE_SPEED_BONUS_AMOUNT,
		UNLIMITED_STACKS,
		SkillRarity::Common,
		100
	});

	m_SkillPool.push_back({
		SkillType::IncreaseMagazineSize,
		L"Magazine Expansion",
		L"Magazine size +5 and current ammo +5.",
		COMMON_MAGAZINE_BONUS,
		0.0f,
		COMMON_MAGAZINE_MAX_STACK,
		SkillRarity::Common,
		100
	});

	m_SkillPool.push_back({
		SkillType::TemporaryInfiniteAmmo,
		L"Infinite Ammo",
		L"For 10 seconds, shooting does not consume ammo.",
		0,
		static_cast<float>(TEMPORARY_INFINITE_AMMO_SECONDS),
		UNLIMITED_STACKS,
		SkillRarity::Rare,
		80
	});

	m_SkillPool.push_back({
		SkillType::IncreaseBulletPierce,
		L"Piercing Rounds",
		L"Bullets pierce 1 additional target.",
		BULLET_PIERCE_BONUS,
		0.0f,
		BULLET_PIERCE_MAX_STACK,
		SkillRarity::Rare,
		80
	});

	m_SkillPool.push_back({
		SkillType::IncreaseItemDropRate,
		L"Lucky Scavenger",
		L"Enemy item drop rate +10%.",
		0,
		RARE_DROP_RATE_BONUS,
		RARE_DROP_RATE_MAX_STACK,
		SkillRarity::Rare,
		70
	});

	m_SkillPool.push_back({
		SkillType::RefillAllAmmo,
		L"Ammo Supply",
		L"Refill current magazine and reserve ammo.",
		0,
		0.0f,
		UNLIMITED_STACKS,
		SkillRarity::Epic,
		40
	});

	m_SkillPool.push_back({
		SkillType::IncreaseBulletDamage,
		L"High-Energy Rounds",
		L"Bullet damage +12.",
		EPIC_BULLET_DAMAGE_BONUS,
		0.0f,
		UNLIMITED_STACKS,
		SkillRarity::Epic,
		40
	});

	m_SkillPool.push_back({
		SkillType::IncreaseMagazineSize,
		L"Large Magazine",
		L"Magazine size +15 and refill current magazine.",
		EPIC_MAGAZINE_BONUS,
		0.0f,
		EPIC_MAGAZINE_MAX_STACK,
		SkillRarity::Epic,
		40
	});

	m_SkillPool.push_back({
		SkillType::IncreaseItemDropRate,
		L"Advanced Scavenger",
		L"Enemy item drop rate +20%.",
		0,
		EPIC_DROP_RATE_BONUS,
		EPIC_DROP_RATE_MAX_STACK,
		SkillRarity::Epic,
		35
	});

	m_SkillPool.push_back({
		SkillType::TemporaryInfiniteAmmo,
		L"Firepower Unleashed",
		L"For 20 seconds, shooting does not consume ammo.",
		0,
		static_cast<float>(LEGENDARY_INFINITE_AMMO_SECONDS),
		UNLIMITED_STACKS,
		SkillRarity::Legendary,
		10
	});
}

void SkillSystem::ResetRuntimeState()
{
	m_SkillStacks.clear();
	m_LastDrawnSkills.clear();
}

std::vector<SkillDefinition> SkillSystem::DrawRandomSkills(std::size_t count)
{
	m_LastDrawnSkills.clear();
	if (m_SkillPool.empty() || count == 0)
	{
		return {};
	}

	std::vector<SkillDefinition> choices;
	std::set<std::wstring> selectedKeys;

	auto isSkillAvailable = [this, &selectedKeys](const SkillDefinition& skill)
	{
		const std::wstring key = BuildSkillStackKey(skill);
		if (selectedKeys.find(key) != selectedKeys.end())
		{
			return false;
		}

		if (skill.maxStack < 0)
		{
			return true;
		}

		const auto found = m_SkillStacks.find(key);
		const int currentStack =
			found != m_SkillStacks.end() ? found->second : 0;
		return currentStack < skill.maxStack;
	};

	auto collectAvailable = [&]()
	{
		std::vector<const SkillDefinition*> available;
		for (const SkillDefinition& skill : m_SkillPool)
		{
			if (isSkillAvailable(skill))
			{
				available.push_back(&skill);
			}
		}
		return available;
	};

	auto rollRarity = [&]()
	{
		int totalWeight = 0;
		for (const RarityWeight& entry : RARITY_WEIGHTS)
		{
			totalWeight += std::max(0, entry.weight);
		}

		std::uniform_int_distribution<int> distribution(1, totalWeight);
		int roll = distribution(m_RandomEngine);
		for (const RarityWeight& entry : RARITY_WEIGHTS)
		{
			roll -= std::max(0, entry.weight);
			if (roll <= 0)
			{
				return entry.rarity;
			}
		}

		return SkillRarity::Common;
	};

	auto chooseWeightedSkill = [&](const std::vector<const SkillDefinition*>& candidates)
		-> const SkillDefinition*
	{
		if (candidates.empty())
		{
			return nullptr;
		}

		int totalWeight = 0;
		for (const SkillDefinition* skill : candidates)
		{
			totalWeight += std::max(0, skill->weight);
		}

		if (totalWeight <= 0)
		{
			std::uniform_int_distribution<std::size_t> distribution(
				0,
				candidates.size() - 1);
			return candidates[distribution(m_RandomEngine)];
		}

		std::uniform_int_distribution<int> distribution(1, totalWeight);
		int roll = distribution(m_RandomEngine);
		for (const SkillDefinition* skill : candidates)
		{
			roll -= std::max(0, skill->weight);
			if (roll <= 0)
			{
				return skill;
			}
		}

		return candidates.back();
	};

	while (choices.size() < count)
	{
		const std::vector<const SkillDefinition*> available = collectAvailable();
		if (available.empty())
		{
			break;
		}

		const SkillRarity targetRarity = rollRarity();
		std::vector<const SkillDefinition*> rarityCandidates;
		for (const SkillDefinition* skill : available)
		{
			if (skill->rarity == targetRarity)
			{
				rarityCandidates.push_back(skill);
			}
		}

		const std::vector<const SkillDefinition*>& candidates =
			rarityCandidates.empty() ? available : rarityCandidates;
		const SkillDefinition* chosenSkill = chooseWeightedSkill(candidates);
		if (!chosenSkill)
		{
			break;
		}

		choices.push_back(*chosenSkill);
		selectedKeys.insert(BuildSkillStackKey(*chosenSkill));
	}

	m_LastDrawnSkills = choices;
	return choices;
}

void SkillSystem::ApplySkill(
	const SkillDefinition& skill,
	PlayerStats& stats)
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

	case SkillType::RefillAllAmmo:
		stats.RefillAllAmmo();
		break;

	case SkillType::IncreaseItemDropRate:
		stats.itemDropRateBonus = std::clamp(
			stats.itemDropRateBonus + ResolveFloatValue(skill),
			0.0f,
			MAX_ITEM_DROP_RATE_BONUS);
		break;

	case SkillType::IncreaseMagazineSize:
		stats.magazineSize += std::max(0, skill.intValue);
		if (skill.rarity == SkillRarity::Epic)
		{
			stats.currentAmmo = stats.magazineSize;
		}
		else
		{
			stats.currentAmmo += std::max(0, skill.intValue);
			stats.currentAmmo = std::min(stats.currentAmmo, stats.magazineSize);
		}
		break;

	default:
		break;
	}

	stats.ClampCurrentHp();
	stats.ClampAmmo();
	++m_SkillStacks[BuildSkillStackKey(skill)];
}
