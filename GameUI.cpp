#include "GameUI.h"

#include "Font.h"
#include "PlayerCharacter.h"
#include "sprite.h"
#include "texture.h"

#include <algorithm>
#include <limits>
#include <string>

namespace
{
	int g_TexWhite = -1;
	std::uint64_t g_RecentExperienceGain = 0;
	float g_ExperienceGainTimer = 0.0f;

	constexpr float EXPERIENCE_GAIN_DISPLAY_TIME = 1.5f;
}

void GameUI_Initialize()
{
	g_TexWhite = Texture_LoadFromFile(L"resource/texture/white.png");
	g_RecentExperienceGain = 0;
	g_ExperienceGainTimer = 0.0f;
}

void GameUI_Update(double elapsedTime)
{
	if (g_ExperienceGainTimer <= 0.0f)
	{
		return;
	}

	g_ExperienceGainTimer -= static_cast<float>(elapsedTime);
	if (g_ExperienceGainTimer <= 0.0f)
	{
		g_ExperienceGainTimer = 0.0f;
		g_RecentExperienceGain = 0;
	}
}

void GameUI_ShowExperienceGain(std::uint64_t amount)
{
	if (amount == 0)
	{
		return;
	}

	// Combine rapid multi-kills into one easy-to-read notification.
	if (amount > std::numeric_limits<std::uint64_t>::max() -
		g_RecentExperienceGain)
	{
		g_RecentExperienceGain =
			std::numeric_limits<std::uint64_t>::max();
	}
	else
	{
		g_RecentExperienceGain += amount;
	}

	g_ExperienceGainTimer = EXPERIENCE_GAIN_DISPLAY_TIME;
}

void GameUI_Draw()
{
	if (g_TexWhite == -1)
	{
		return;
	}

	PlayerCharacter* player = Player_GetInstance();
	if (!player)
	{
		return;
	}

	const float x = 20.0f;
	const float y = 20.0f;
	const float width = 200.0f;

	// Health bar.
	const float healthHeight = 20.0f;
	const float healthRatio =
		std::clamp(player->GetHP() / 100.0f, 0.0f, 1.0f);

	Sprite_Draw(
		g_TexWhite,
		x - 2.0f,
		y - 2.0f,
		width + 4.0f,
		healthHeight + 4.0f,
		{ 0.0f, 0.0f, 0.0f, 1.0f });
	Sprite_Draw(
		g_TexWhite,
		x,
		y,
		width * healthRatio,
		healthHeight,
		{ 1.0f, 0.0f, 0.0f, 1.0f });

	// Experience bar.
	const ExperienceComponent& experience =
		player->GetExperienceComponent();
	const std::uint64_t currentExperience =
		experience.GetCurrentExperience();
	const std::uint64_t requiredExperience =
		experience.GetRequiredExperience();

	const float experienceRatio = requiredExperience > 0
		? std::clamp(
			static_cast<float>(
				static_cast<double>(currentExperience) /
				static_cast<double>(requiredExperience)),
			0.0f,
			1.0f)
		: 1.0f;

	const float experienceY = y + healthHeight + 12.0f;
	const float experienceHeight = 14.0f;

	Sprite_Draw(
		g_TexWhite,
		x - 2.0f,
		experienceY - 2.0f,
		width + 4.0f,
		experienceHeight + 4.0f,
		{ 0.0f, 0.0f, 0.0f, 0.9f });
	Sprite_Draw(
		g_TexWhite,
		x,
		experienceY,
		width * experienceRatio,
		experienceHeight,
		{ 0.15f, 0.65f, 1.0f, 1.0f });

	std::wstring experienceText =
		L"LV " + std::to_wstring(experience.GetLevel()) +
		L"  EXP " + std::to_wstring(currentExperience);
	if (requiredExperience > 0)
	{
		experienceText += L" / " + std::to_wstring(requiredExperience);
	}
	else
	{
		experienceText += L" MAX";
	}

	Font_Draw(
		experienceText.c_str(),
		x,
		experienceY + experienceHeight + 4.0f,
		{ 0.75f, 0.9f, 1.0f, 1.0f });

	// Short "+EXP" notification after a kill.
	if (g_ExperienceGainTimer > 0.0f &&
		g_RecentExperienceGain > 0)
	{
		const float alpha =
			std::clamp(g_ExperienceGainTimer / 0.5f, 0.0f, 1.0f);
		const std::wstring gainText =
			L"+" + std::to_wstring(g_RecentExperienceGain) + L" EXP";

		Font_Draw(
			gainText.c_str(),
			x,
			experienceY + experienceHeight + 42.0f,
			{ 0.3f, 1.0f, 0.45f, alpha });
	}
}
