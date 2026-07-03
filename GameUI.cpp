#include "GameUI.h"

#include "direct3d.h"
#include "PlayerCharacter.h"
#include "sprite.h"
#include "texture.h"
#include "UIFont.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace
{
	int g_TexWhite = -1;
	std::uint64_t g_RecentExperienceGain = 0;
	float g_ExperienceGainTimer = 0.0f;

	constexpr float EXPERIENCE_GAIN_DISPLAY_TIME = 1.5f;
	constexpr float HUD_FONT_SCALE = 0.54f;
	constexpr float EXP_GAIN_FONT_SCALE = 0.58f;
	constexpr float INFINITE_AMMO_TIMER_SCALE = 0.68f;
	constexpr float INFINITE_AMMO_HEAD_HEIGHT = 2.55f;
	constexpr float INFINITE_AMMO_SCREEN_Y_OFFSET = -28.0f;

	void DrawInfiniteAmmoWorldTimer(
		const PlayerCharacter& player,
		const DirectX::XMMATRIX& view,
		const DirectX::XMMATRIX& proj)
	{
		const PlayerStats& stats = player.GetStats();
		if (!stats.IsInfiniteAmmoActive() ||
			stats.infiniteAmmoTimer <= 0.0f)
		{
			return;
		}

		D3D11_VIEWPORT viewport{};
		UINT viewportCount = 1;
		Direct3D_GetDeviceContext()->RSGetViewports(&viewportCount, &viewport);
		if (viewportCount == 0 ||
			viewport.Width <= 0.0f ||
			viewport.Height <= 0.0f)
		{
			return;
		}

		DirectX::XMFLOAT3 playerPosition = player.GetPosition();
		playerPosition.y += INFINITE_AMMO_HEAD_HEIGHT;

		const DirectX::XMVECTOR screenPosition =
			DirectX::XMVector3Project(
				DirectX::XMLoadFloat3(&playerPosition),
				viewport.TopLeftX,
				viewport.TopLeftY,
				viewport.Width,
				viewport.Height,
				viewport.MinDepth,
				viewport.MaxDepth,
				proj,
				view,
				DirectX::XMMatrixIdentity());

		const float screenZ = DirectX::XMVectorGetZ(screenPosition);
		if (screenZ < 0.0f || screenZ > 1.0f)
		{
			return;
		}

		std::wostringstream stream;
		stream << std::fixed << std::setprecision(1)
			<< stats.infiniteAmmoTimer << L"s";
		const std::wstring timerText = stream.str();

		const float textWidth =
			UIFont_MeasureText(timerText.c_str(), INFINITE_AMMO_TIMER_SCALE);
		const float screenX =
			DirectX::XMVectorGetX(screenPosition) - textWidth * 0.5f;
		const float screenY =
			DirectX::XMVectorGetY(screenPosition) + INFINITE_AMMO_SCREEN_Y_OFFSET;

		UIFont_Draw(
			timerText.c_str(),
			screenX + 2.0f,
			screenY + 2.0f,
			INFINITE_AMMO_TIMER_SCALE,
			{ 0.0f, 0.0f, 0.0f, 0.85f });
		UIFont_Draw(
			timerText.c_str(),
			screenX,
			screenY,
			INFINITE_AMMO_TIMER_SCALE,
			{ 1.0f, 0.05f, 0.05f, 1.0f });
	}
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

void GameUI_Draw(
	const DirectX::XMMATRIX& view,
	const DirectX::XMMATRIX& proj)
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
	const PlayerStats& stats = player->GetStats();
	const float healthRatio = stats.maxHp > 0.0f
		? std::clamp(stats.currentHp / stats.maxHp, 0.0f, 1.0f)
		: 0.0f;

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
	const PlayerExp& experience = player->GetPlayerExp();
	const std::uint64_t currentExperience =
		experience.currentExp;
	const std::uint64_t requiredExperience =
		experience.expToNextLevel;

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
		L"LV " + std::to_wstring(experience.level) +
		L"  EXP " + std::to_wstring(currentExperience);
	if (requiredExperience > 0)
	{
		experienceText += L" / " + std::to_wstring(requiredExperience);
	}
	else
	{
		experienceText += L" MAX";
	}

	UIFont_Draw(
		experienceText.c_str(),
		x,
		experienceY + experienceHeight + 4.0f,
		HUD_FONT_SCALE,
		{ 0.75f, 0.9f, 1.0f, 1.0f });

	// Short "+EXP" notification after a kill.
	if (g_ExperienceGainTimer > 0.0f &&
		g_RecentExperienceGain > 0)
	{
		const float alpha =
			std::clamp(g_ExperienceGainTimer / 0.5f, 0.0f, 1.0f);
		const std::wstring gainText =
			L"+" + std::to_wstring(g_RecentExperienceGain) + L" EXP";

		UIFont_Draw(
			gainText.c_str(),
			x,
			experienceY + experienceHeight + 30.0f,
			EXP_GAIN_FONT_SCALE,
			{ 0.3f, 1.0f, 0.45f, alpha });
	}

	DrawInfiniteAmmoWorldTimer(*player, view, proj);
}
