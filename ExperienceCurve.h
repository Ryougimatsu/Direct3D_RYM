#pragma once

#include <cstdint>

// Shared level-up requirement curve for both the legacy EXP HUD component and
// the roguelike level-up selection state.  Levels 1-10 are intentionally fast;
// after that the required EXP ramps up more aggressively.
std::uint64_t ExperienceCurve_CalculateRequiredExperience(
	std::uint32_t targetLevel);
