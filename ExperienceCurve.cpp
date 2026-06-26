#include "ExperienceCurve.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	constexpr std::uint32_t EARLY_FAST_LEVEL_END = 10;

	constexpr long double EARLY_BASE_EXP = 25.0L;
	constexpr long double EARLY_LINEAR_EXP = 10.0L;
	constexpr long double EARLY_QUADRATIC_EXP = 1.5L;

	constexpr long double LATE_LINEAR_EXP = 120.0L;
	constexpr long double LATE_QUADRATIC_EXP = 45.0L;
	constexpr long double LATE_POWER_EXP = 35.0L;
	constexpr long double LATE_POWER_RATE = 2.25L;

	std::uint64_t RoundAndClampExp(long double value)
	{
		const long double maxValue =
			static_cast<long double>(
				std::numeric_limits<std::uint64_t>::max());
		if (!std::isfinite(value) || value >= maxValue)
		{
			return std::numeric_limits<std::uint64_t>::max();
		}

		if (value <= 0.0L)
		{
			return 1;
		}

		return static_cast<std::uint64_t>(std::floor(value + 0.5L));
	}

	long double CalculateEarlyRequiredExperience(std::uint32_t level)
	{
		const long double value = static_cast<long double>(level);
		return EARLY_BASE_EXP +
			EARLY_LINEAR_EXP * value +
			EARLY_QUADRATIC_EXP * value * value;
	}
}

std::uint64_t ExperienceCurve_CalculateRequiredExperience(
	std::uint32_t targetLevel)
{
	targetLevel = std::max<std::uint32_t>(1, targetLevel);

	if (targetLevel <= EARLY_FAST_LEVEL_END)
	{
		return RoundAndClampExp(
			CalculateEarlyRequiredExperience(targetLevel));
	}

	const long double lateLevel =
		static_cast<long double>(targetLevel - EARLY_FAST_LEVEL_END);
	const long double required =
		CalculateEarlyRequiredExperience(EARLY_FAST_LEVEL_END) +
		LATE_LINEAR_EXP * lateLevel +
		LATE_QUADRATIC_EXP * lateLevel * lateLevel +
		LATE_POWER_EXP * std::pow(lateLevel, LATE_POWER_RATE);

	return RoundAndClampExp(required);
}
