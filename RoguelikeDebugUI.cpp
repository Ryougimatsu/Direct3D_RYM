#include "RoguelikeDebugUI.h"

#include "direct3d.h"
#include "DifficultyManager.h"
#include "PlayerCharacter.h"
#include "PlayerExp.h"
#include "PlayerStats.h"
#include "sprite.h"
#include "texture.h"
#include "UIFont.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{
	int g_TexWhite = -1;

	std::wstring FormatFloat(float value, int precision = 1)
	{
		std::wostringstream stream;
		stream << std::fixed << std::setprecision(precision) << value;
		return stream.str();
	}

	void DrawBackground(float x, float y, float width, float height)
	{
		if (g_TexWhite == -1)
		{
			return;
		}

		Sprite_Draw(
			g_TexWhite,
			x,
			y,
			width,
			height,
			{ 0.0f, 0.0f, 0.0f, 0.52f });
	}

	const wchar_t* GetRarityLabel(SkillRarity rarity)
	{
		switch (rarity)
		{
		case SkillRarity::Common:
			return L"COMMON";
		case SkillRarity::Rare:
			return L"RARE";
		case SkillRarity::Epic:
			return L"EPIC";
		case SkillRarity::Legendary:
			return L"LEGEND";
		default:
			return L"COMMON";
		}
	}
}

void RoguelikeDebugUI_Initialize()
{
	g_TexWhite = Texture_LoadFromFile(L"resource/texture/white.png");
}

void RoguelikeDebugUI_Finalize()
{
	g_TexWhite = -1;
}

void RoguelikeDebugUI_Draw(
	const PlayerCharacter& player,
	const DifficultyManager& difficultyManager,
	const std::vector<SkillDefinition>& lastLevelUpOptions)
{
	const PlayerStats& stats = player.GetStats();
	const PlayerExp& exp = player.GetPlayerExp();

	const float x = 18.0f;
	const float y =
		std::max(
			8.0f,
			static_cast<float>(Direct3D_GetBackBufferHeight()) - 516.0f);
	constexpr float FONT_SCALE = 0.42f;
	const float lineHeight = 18.0f;

	DrawBackground(x - 8.0f, y - 8.0f, 336.0f, 500.0f);

	const DirectX::XMFLOAT4 titleColor{ 1.0f, 0.86f, 0.25f, 1.0f };
	const DirectX::XMFLOAT4 textColor{ 0.82f, 0.92f, 1.0f, 1.0f };
	const DirectX::XMFLOAT4 activeColor{ 0.35f, 1.0f, 0.45f, 1.0f };
	const DirectX::XMFLOAT4 inactiveColor{ 0.75f, 0.78f, 0.85f, 1.0f };

	float drawY = y;
	UIFont_Draw(L"Roguelike Debug  [F3]", x, drawY, FONT_SCALE, titleColor);
	drawY += lineHeight;

	const std::wstring levelText =
		L"Level: " + std::to_wstring(exp.level);
	UIFont_Draw(levelText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring expText =
		L"EXP: " + std::to_wstring(exp.currentExp) +
		L" / " + std::to_wstring(exp.expToNextLevel);
	UIFont_Draw(expText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring hpText =
		L"HP: " + FormatFloat(stats.currentHp, 0) +
		L" / " + FormatFloat(stats.maxHp, 0);
	UIFont_Draw(hpText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring damageText =
		L"Bullet Damage: " + FormatFloat(stats.bulletDamage, 1);
	UIFont_Draw(damageText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring pierceText =
		L"Bullet Pierce: " + std::to_wstring(stats.bulletPierce);
	UIFont_Draw(pierceText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring speedText =
		L"Move Speed: " + FormatFloat(stats.moveSpeed, 1);
	UIFont_Draw(speedText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const bool infiniteAmmoActive = stats.IsInfiniteAmmoActive();
	const std::wstring infiniteAmmoText =
		std::wstring(L"Infinite Ammo: ") +
		(infiniteAmmoActive ? L"ON" : L"OFF");
	UIFont_Draw(
		infiniteAmmoText.c_str(),
		x,
		drawY,
		FONT_SCALE,
		infiniteAmmoActive ? activeColor : inactiveColor);
	drawY += lineHeight;

	const std::wstring infiniteAmmoTimerText =
		L"Ammo Timer: " + FormatFloat(stats.infiniteAmmoTimer, 1) + L"s";
	UIFont_Draw(
		infiniteAmmoTimerText.c_str(),
		x,
		drawY,
		FONT_SCALE,
		infiniteAmmoActive ? activeColor : inactiveColor);
	drawY += lineHeight;

	drawY += 6.0f;
	UIFont_Draw(L"Ammo / Drop", x, drawY, FONT_SCALE, titleColor);
	drawY += lineHeight;

	const std::wstring ammoText =
		L"Ammo: " + std::to_wstring(stats.currentAmmo) +
		L" / " + std::to_wstring(stats.magazineSize);
	UIFont_Draw(ammoText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring reserveText =
		L"Reserve: " + std::to_wstring(stats.reserveAmmo) +
		L" / " + std::to_wstring(stats.maxReserveAmmo);
	UIFont_Draw(reserveText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring dropBonusText =
		L"Drop Bonus: " + FormatFloat(stats.itemDropRateBonus * 100.0f, 0) + L"%";
	UIFont_Draw(dropBonusText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	drawY += 6.0f;
	UIFont_Draw(L"Difficulty", x, drawY, FONT_SCALE, titleColor);
	drawY += lineHeight;

	const std::wstring difficultyTimeText =
		L"Time: " + FormatFloat(difficultyManager.GetElapsedTime(), 1) + L"s";
	UIFont_Draw(difficultyTimeText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring difficultyLevelText =
		L"Difficulty Lv: " + std::to_wstring(difficultyManager.GetDifficultyLevel());
	UIFont_Draw(difficultyLevelText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring enemyHpText =
		L"Enemy HP x" + FormatFloat(difficultyManager.GetEnemyHpMultiplier(), 2);
	UIFont_Draw(enemyHpText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring enemyDamageText =
		L"Enemy DMG x" + FormatFloat(difficultyManager.GetEnemyDamageMultiplier(), 2);
	UIFont_Draw(enemyDamageText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring enemySpeedText =
		L"Enemy SPD x" + FormatFloat(difficultyManager.GetEnemySpeedMultiplier(), 2);
	UIFont_Draw(enemySpeedText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring spawnIntervalText =
		L"Spawn: " + FormatFloat(difficultyManager.GetSpawnInterval(), 2) + L"s";
	UIFont_Draw(spawnIntervalText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	const std::wstring enemiesPerWaveText =
		L"Enemies/Wave: " + std::to_wstring(difficultyManager.GetEnemiesPerWave());
	UIFont_Draw(enemiesPerWaveText.c_str(), x, drawY, FONT_SCALE, textColor);
	drawY += lineHeight;

	drawY += 6.0f;
	UIFont_Draw(L"Last Choices", x, drawY, FONT_SCALE, titleColor);
	drawY += lineHeight;

	std::wstring rarityText = L"Rarity:";
	if (lastLevelUpOptions.empty())
	{
		rarityText += L" -";
	}
	else
	{
		for (const SkillDefinition& skill : lastLevelUpOptions)
		{
			rarityText += L" [";
			rarityText += GetRarityLabel(skill.rarity);
			rarityText += L"]";
		}
	}
	UIFont_Draw(rarityText.c_str(), x, drawY, FONT_SCALE, textColor);
}
