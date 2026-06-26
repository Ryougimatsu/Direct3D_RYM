#pragma once

#include "PlayerStats.h"
#include "SkillDefinition.h"

#include <cstddef>
#include <random>
#include <vector>

// SkillSystem only owns skill definitions and applies their numeric effects to
// PlayerStats.  UI, GameState, bullets, and enemies should stay outside it.
class SkillSystem
{
public:
	SkillSystem();

	void Initialize();
	std::vector<SkillDefinition> DrawRandomSkills(std::size_t count = 3);
	void ApplySkill(const SkillDefinition& skill, PlayerStats& stats) const;

	const std::vector<SkillDefinition>& GetSkillPool() const
	{
		return m_SkillPool;
	}

private:
	std::vector<SkillDefinition> m_SkillPool;
	std::mt19937 m_RandomEngine;
};
