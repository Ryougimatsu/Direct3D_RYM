#pragma once

#include <cstdint>

// Roguelike upgrade EXP state.  This intentionally separates "EXP reached a
// level-up threshold" from "the player has chosen a skill", so LevelUpUI can
// consume one pending level-up after the player picks from three options.
struct PlayerExp
{
	std::uint32_t level = 1;
	std::uint64_t currentExp = 0;
	std::uint64_t expToNextLevel = 100;

	void Reset(
		std::uint32_t startLevel = 1,
		std::uint64_t startExp = 0);
	void AddExp(std::uint64_t amount);
	bool CanLevelUp() const;
	bool ConsumeLevelUp();

	static std::uint64_t CalculateExpToNextLevel(std::uint32_t targetLevel);
};
