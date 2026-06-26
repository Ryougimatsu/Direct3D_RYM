#include "PlayerExp.h"

#include "ExperienceCurve.h"

#include <algorithm>
#include <limits>

void PlayerExp::Reset(
	std::uint32_t startLevel,
	std::uint64_t startExp)
{
	level = std::max<std::uint32_t>(1, startLevel);
	currentExp = startExp;
	expToNextLevel = CalculateExpToNextLevel(level);
}

void PlayerExp::AddExp(std::uint64_t amount)
{
	if (amount == 0)
	{
		return;
	}

	const std::uint64_t maxExp =
		std::numeric_limits<std::uint64_t>::max();
	if (amount > maxExp - currentExp)
	{
		currentExp = maxExp;
	}
	else
	{
		currentExp += amount;
	}
}

bool PlayerExp::CanLevelUp() const
{
	return expToNextLevel > 0 && currentExp >= expToNextLevel;
}

bool PlayerExp::ConsumeLevelUp()
{
	if (!CanLevelUp())
	{
		return false;
	}

	currentExp -= expToNextLevel;
	++level;
	expToNextLevel = CalculateExpToNextLevel(level);
	return true;
}

std::uint64_t PlayerExp::CalculateExpToNextLevel(std::uint32_t targetLevel)
{
	return ExperienceCurve_CalculateRequiredExperience(targetLevel);
}
