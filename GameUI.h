#pragma once

#include <cstdint>

void GameUI_Initialize();
void GameUI_Update(double elapsedTime);
void GameUI_Draw();

void GameUI_ShowExperienceGain(std::uint64_t amount);
