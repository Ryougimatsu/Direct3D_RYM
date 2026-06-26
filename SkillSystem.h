#pragma once

#include "PlayerStats.h"
#include "SkillDefinition.h"

#include <cstddef>
#include <map>
#include <random>
#include <string>
#include <vector>

// SkillSystem only owns skill definitions and applies their numeric effects to
// PlayerStats.  UI, GameState, bullets, and enemies should stay outside it.
class SkillSystem
{
public:
	SkillSystem();

	void Initialize();
	void ResetRuntimeState();
	std::vector<SkillDefinition> DrawRandomSkills(std::size_t count = 3);
	void ApplySkill(const SkillDefinition& skill, PlayerStats& stats);

	const std::vector<SkillDefinition>& GetSkillPool() const
	{
		return m_SkillPool;
	}

	const std::vector<SkillDefinition>& GetLastDrawnSkills() const
	{
		return m_LastDrawnSkills;
	}

private:
	std::vector<SkillDefinition> m_SkillPool;
	std::vector<SkillDefinition> m_LastDrawnSkills;
	std::map<std::wstring, int> m_SkillStacks;
	std::mt19937 m_RandomEngine;
};
