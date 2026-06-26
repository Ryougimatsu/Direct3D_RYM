#include "LevelUpUI.h"

#include "direct3d.h"
#include "sprite.h"
#include "texture.h"
#include "UIFont.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace
{
	int g_TexWhite = -1;

	const DirectX::XMFLOAT4 COLOR_OVERLAY{ 0.0f, 0.0f, 0.0f, 0.74f };
	const DirectX::XMFLOAT4 COLOR_PANEL{ 0.05f, 0.07f, 0.11f, 0.97f };
	const DirectX::XMFLOAT4 COLOR_CARD{ 0.11f, 0.13f, 0.20f, 0.95f };
	const DirectX::XMFLOAT4 COLOR_CARD_SELECTED{ 0.20f, 0.24f, 0.38f, 0.98f };
	const DirectX::XMFLOAT4 COLOR_BORDER{ 0.25f, 0.32f, 0.48f, 1.0f };
	const DirectX::XMFLOAT4 COLOR_BORDER_SELECTED{ 1.0f, 0.82f, 0.22f, 1.0f };
	const DirectX::XMFLOAT4 COLOR_TITLE{ 1.0f, 0.88f, 0.28f, 1.0f };
	const DirectX::XMFLOAT4 COLOR_TEXT{ 0.94f, 0.96f, 1.0f, 1.0f };
	const DirectX::XMFLOAT4 COLOR_DESCRIPTION{ 0.72f, 0.80f, 0.92f, 1.0f };
	const DirectX::XMFLOAT4 COLOR_HINT{ 0.68f, 0.78f, 0.95f, 1.0f };

	constexpr int MAX_VISIBLE_OPTIONS = 3;
	constexpr float PANEL_HORIZONTAL_MARGIN = 160.0f;
	constexpr float PANEL_WIDTH_RATIO = 0.70f;
	constexpr float PANEL_MIN_WIDTH = 880.0f;
	constexpr float PANEL_MAX_WIDTH = 1220.0f;
	constexpr float PANEL_HEIGHT_RATIO = 0.44f;
	constexpr float PANEL_MIN_HEIGHT = 400.0f;
	constexpr float PANEL_MAX_HEIGHT = 500.0f;
	constexpr float TITLE_SCALE = 0.82f;
	constexpr float CARD_NAME_SCALE = 0.58f;
	constexpr float CARD_DESCRIPTION_SCALE = 0.52f;
	constexpr float HINT_SCALE = 0.50f;

	void DrawSolidRect(
		float x,
		float y,
		float width,
		float height,
		const DirectX::XMFLOAT4& color)
	{
		if (g_TexWhite == -1)
		{
			return;
		}

		Sprite_Draw(g_TexWhite, x, y, width, height, color);
	}

	void DrawFrame(
		float x,
		float y,
		float width,
		float height,
		float thickness,
		const DirectX::XMFLOAT4& color)
	{
		DrawSolidRect(x, y, width, thickness, color);
		DrawSolidRect(x, y + height - thickness, width, thickness, color);
		DrawSolidRect(x, y, thickness, height, color);
		DrawSolidRect(x + width - thickness, y, thickness, height, color);
	}

	const wchar_t* GetRarityLabel(SkillRarity rarity)
	{
		switch (rarity)
		{
		case SkillRarity::Common:
			return L"[COMMON]";
		case SkillRarity::Rare:
			return L"[RARE]";
		case SkillRarity::Epic:
			return L"[EPIC]";
		case SkillRarity::Legendary:
			return L"[LEGENDARY]";
		default:
			return L"[COMMON]";
		}
	}

	DirectX::XMFLOAT4 GetRarityColor(SkillRarity rarity)
	{
		switch (rarity)
		{
		case SkillRarity::Common:
			return { 0.86f, 0.88f, 0.92f, 1.0f };
		case SkillRarity::Rare:
			return { 0.25f, 0.55f, 1.0f, 1.0f };
		case SkillRarity::Epic:
			return { 0.72f, 0.32f, 1.0f, 1.0f };
		case SkillRarity::Legendary:
			return { 1.0f, 0.72f, 0.18f, 1.0f };
		default:
			return COLOR_TEXT;
		}
	}
}

void LevelUpUI_Initialize()
{
	g_TexWhite = Texture_LoadFromFile(L"resource/texture/white.png");
}

void LevelUpUI_Finalize()
{
	g_TexWhite = -1;
}

void LevelUpUI_Draw(
	const std::vector<SkillDefinition>& options,
	int selectedOptionIndex)
{
	if (g_TexWhite == -1 || options.empty())
	{
		return;
	}

	const float screenW =
		static_cast<float>(Direct3D_GetBackBufferWidth());
	const float screenH =
		static_cast<float>(Direct3D_GetBackBufferHeight());

	DrawSolidRect(0.0f, 0.0f, screenW, screenH, COLOR_OVERLAY);

	const float maxPanelW =
		std::max(320.0f, screenW - PANEL_HORIZONTAL_MARGIN);
	const float minPanelW = std::min(PANEL_MIN_WIDTH, maxPanelW);
	const float panelW = std::clamp(
		screenW * PANEL_WIDTH_RATIO,
		minPanelW,
		std::min(PANEL_MAX_WIDTH, maxPanelW));
	const float maxPanelH = std::max(300.0f, screenH - 80.0f);
	const float minPanelH = std::min(PANEL_MIN_HEIGHT, maxPanelH);
	const float panelH = std::clamp(
		screenH * PANEL_HEIGHT_RATIO,
		minPanelH,
		std::min(PANEL_MAX_HEIGHT, maxPanelH));
	const float panelX = (screenW - panelW) * 0.5f;
	const float panelY = (screenH - panelH) * 0.5f;

	DrawSolidRect(panelX, panelY, panelW, panelH, COLOR_PANEL);
	DrawFrame(panelX, panelY, panelW, panelH, 3.0f, COLOR_BORDER);

	const wchar_t* titleText = L"LEVEL UP";
	UIFont_Draw(
		titleText,
		panelX + (panelW - UIFont_MeasureText(titleText, TITLE_SCALE)) * 0.5f,
		panelY + panelH * 0.07f,
		TITLE_SCALE,
		COLOR_TITLE);

	const int optionCount =
		static_cast<int>(
			std::min<std::size_t>(options.size(), MAX_VISIBLE_OPTIONS));
	selectedOptionIndex =
		std::clamp(selectedOptionIndex, 0, optionCount - 1);

	const float gap = 24.0f;
	const float cardW =
		(panelW - gap * (MAX_VISIBLE_OPTIONS + 1.0f)) /
		MAX_VISIBLE_OPTIONS;
	const float cardH = panelH * 0.52f;
	const float cardY = panelY + panelH * 0.27f;

	for (int i = 0; i < optionCount; ++i)
	{
		const bool selected = (i == selectedOptionIndex);
		const float cardX = panelX + gap + (cardW + gap) * i;

		DrawSolidRect(
			cardX,
			cardY,
			cardW,
			cardH,
			selected ? COLOR_CARD_SELECTED : COLOR_CARD);
		DrawFrame(
			cardX,
			cardY,
			cardW,
			cardH,
			selected ? 5.0f : 2.0f,
			selected ? COLOR_BORDER_SELECTED : GetRarityColor(options[i].rarity));

		const SkillDefinition& skill = options[i];
		const DirectX::XMFLOAT4 rarityColor = GetRarityColor(skill.rarity);
		UIFont_Draw(
			GetRarityLabel(skill.rarity),
			cardX + 18.0f,
			cardY + 16.0f,
			CARD_DESCRIPTION_SCALE,
			rarityColor);

		UIFont_DrawWrapped(
			skill.name.c_str(),
			cardX + 18.0f,
			cardY + 48.0f,
			cardW - 36.0f,
			CARD_NAME_SCALE,
			selected ? COLOR_TITLE : rarityColor);

		UIFont_DrawWrapped(
			skill.description.c_str(),
			cardX + 18.0f,
			cardY + 112.0f,
			cardW - 36.0f,
			CARD_DESCRIPTION_SCALE,
			COLOR_DESCRIPTION);

		const std::wstring numberLabel =
			L"[" + std::to_wstring(i + 1) + L"]";
		UIFont_Draw(
			numberLabel.c_str(),
			cardX + cardW - 46.0f,
			cardY + cardH - 38.0f,
			CARD_DESCRIPTION_SCALE,
			selected ? COLOR_BORDER_SELECTED : COLOR_HINT);
	}

	const wchar_t* hintText =
		L"Left / A: Select    Right / D: Select    Enter: Confirm";
	UIFont_Draw(
		hintText,
		panelX + (panelW - UIFont_MeasureText(hintText, HINT_SCALE)) * 0.5f,
		panelY + panelH - 48.0f,
		HINT_SCALE,
		COLOR_HINT);
}
