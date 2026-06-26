#pragma once

#include "SkillDefinition.h"

#include <vector>

class PlayerCharacter;
class DifficultyManager;

void RoguelikeDebugUI_Initialize();
void RoguelikeDebugUI_Finalize();
void RoguelikeDebugUI_Draw(
	const PlayerCharacter& player,
	const DifficultyManager& difficultyManager,
	const std::vector<SkillDefinition>& lastLevelUpOptions);
