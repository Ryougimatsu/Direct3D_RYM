#include "ExperienceComponent.h"

#include "ExperienceCurve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace
{
	constexpr std::uint64_t kMaximumExperience =
		std::numeric_limits<std::uint64_t>::max();

	std::uint64_t RoundAndClampExperience(long double value)
	{
		if (!std::isfinite(value) ||
			value >= static_cast<long double>(kMaximumExperience))
		{
			return kMaximumExperience;
		}

		if (value <= 0.0L)
		{
			return 0;
		}

		return static_cast<std::uint64_t>(std::floor(value + 0.5L));
	}
}

ExperienceComponent::ExperienceComponent(ExperienceConfig config)
	: m_config(std::move(config))
{
	SanitizeConfig();
}

ExperienceGainResult ExperienceComponent::AddExperience(
	std::uint64_t enemyBaseExperience,
	std::uint32_t enemyLevel)
{
	const double levelFactor = CalculateLevelFactor(enemyLevel);
	const std::uint64_t reward =
		CalculateExperienceReward(enemyBaseExperience, enemyLevel);

	return ApplyExperience(reward, enemyBaseExperience, levelFactor);
}

ExperienceGainResult ExperienceComponent::AddRawExperience(std::uint64_t amount)
{
	return ApplyExperience(amount, amount, 1.0);
}

std::uint64_t ExperienceComponent::CalculateExperienceReward(
	std::uint64_t enemyBaseExperience,
	std::uint32_t enemyLevel) const
{
	if (enemyBaseExperience == 0)
	{
		return 0;
	}

	const long double reward =
		static_cast<long double>(enemyBaseExperience) *
		static_cast<long double>(CalculateLevelFactor(enemyLevel)) *
		static_cast<long double>(m_config.experienceMultiplier);

	// A defeated enemy with a positive base reward always grants at least 1 EXP.
	return std::max<std::uint64_t>(1, RoundAndClampExperience(reward));
}

double ExperienceComponent::CalculateLevelFactor(std::uint32_t enemyLevel) const
{
	enemyLevel = std::max<std::uint32_t>(1, enemyLevel);
	const std::int64_t levelDifference =
		static_cast<std::int64_t>(enemyLevel) -
		static_cast<std::int64_t>(m_level);

	double factor = 1.0;
	if (levelDifference >= 0)
	{
		factor += static_cast<double>(levelDifference) *
			m_config.higherLevelBonusPerLevel;
	}
	else
	{
		const double levelGap = static_cast<double>(-levelDifference);
		factor = 1.0 /
			(1.0 + levelGap * m_config.lowerLevelPenaltyPerLevel);
	}

	return std::clamp(
		factor,
		m_config.minimumLevelFactor,
		m_config.maximumLevelFactor);
}

std::uint64_t ExperienceComponent::CalculateRequiredExperience(
	std::uint32_t level) const
{
	return ExperienceCurve_CalculateRequiredExperience(level);
}

void ExperienceComponent::SetLevelUpCallback(LevelUpCallback callback)
{
	m_levelUpCallback = std::move(callback);
}

void ExperienceComponent::SetExperienceMultiplier(double multiplier)
{
	m_config.experienceMultiplier = std::max(0.0, multiplier);
}

void ExperienceComponent::Reset(
	std::uint32_t level,
	std::uint64_t currentExperience)
{
	m_level = std::max<std::uint32_t>(1, level);
	if (m_config.maximumLevel > 0)
	{
		m_level = std::min(m_level, m_config.maximumLevel);
	}
	m_currentExperience = currentExperience;
}

std::uint64_t ExperienceComponent::GetRequiredExperience() const
{
	return IsMaxLevel() ? 0 : CalculateRequiredExperience(m_level);
}

bool ExperienceComponent::IsMaxLevel() const
{
	return m_level == std::numeric_limits<std::uint32_t>::max() ||
		(m_config.maximumLevel > 0 && m_level >= m_config.maximumLevel);
}

ExperienceGainResult ExperienceComponent::ApplyExperience(
	std::uint64_t amount,
	std::uint64_t baseExperience,
	double levelFactor)
{
	ExperienceGainResult result;
	result.baseExperience = baseExperience;
	result.awardedExperience = amount;
	result.levelFactor = levelFactor;
	result.previousLevel = m_level;
	result.newLevel = m_level;

	if (amount == 0 || IsMaxLevel())
	{
		return result;
	}

	if (amount > kMaximumExperience - m_currentExperience)
	{
		m_currentExperience = kMaximumExperience;
	}
	else
	{
		m_currentExperience += amount;
	}

	while (!IsMaxLevel())
	{
		const std::uint64_t requiredExperience = GetRequiredExperience();
		if (m_currentExperience < requiredExperience)
		{
			break;
		}

		m_currentExperience -= requiredExperience;

		const std::uint32_t previousLevel = m_level;
		++m_level;

		LevelUpEvent event;
		event.previousLevel = previousLevel;
		event.newLevel = m_level;
		event.remainingExperience = m_currentExperience;
		event.experienceRequiredForNextLevel = GetRequiredExperience();
		result.levelUps.push_back(event);
	}

	result.newLevel = m_level;

	// Invoke callbacks after state calculation so callback code sees a stable result.
	if (m_levelUpCallback)
	{
		const LevelUpCallback callback = m_levelUpCallback;
		for (const LevelUpEvent& event : result.levelUps)
		{
			callback(event);
		}
	}

	return result;
}

void ExperienceComponent::SanitizeConfig()
{
	m_config.baseRequiredExperience =
		std::max<std::uint64_t>(1, m_config.baseRequiredExperience);
	m_config.growthRate = std::max(0.0, m_config.growthRate);
	m_config.experienceMultiplier =
		std::max(0.0, m_config.experienceMultiplier);
	m_config.higherLevelBonusPerLevel =
		std::max(0.0, m_config.higherLevelBonusPerLevel);
	m_config.lowerLevelPenaltyPerLevel =
		std::max(0.0, m_config.lowerLevelPenaltyPerLevel);
	m_config.minimumLevelFactor =
		std::max(0.0, m_config.minimumLevelFactor);
	m_config.maximumLevelFactor =
		std::max(m_config.minimumLevelFactor, m_config.maximumLevelFactor);
}
