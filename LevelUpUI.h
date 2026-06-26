#pragma once

#include "SkillDefinition.h"

#include <vector>

void LevelUpUI_Initialize();
void LevelUpUI_Finalize();
void LevelUpUI_Draw(
	const std::vector<SkillDefinition>& options,
	int selectedOptionIndex);
