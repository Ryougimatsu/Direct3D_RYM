#pragma once

class PlayerCharacter;
class DifficultyManager;

void RoguelikeDebugUI_Initialize();
void RoguelikeDebugUI_Finalize();
void RoguelikeDebugUI_Draw(const PlayerCharacter& player, const DifficultyManager& difficultyManager);
