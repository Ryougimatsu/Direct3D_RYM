#pragma once

#include <cstdint>
#include <DirectXMath.h>

void GameUI_Initialize();
void GameUI_Update(double elapsedTime);
void GameUI_Draw(
	const DirectX::XMMATRIX& view,
	const DirectX::XMMATRIX& proj);

void GameUI_ShowExperienceGain(std::uint64_t amount);
