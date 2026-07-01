#include "LevelUpUI.h"

#include "direct3d.h"
#include "UIDraw.h"
#include "UIFont.h"
#include "UITheme.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace
{
	constexpr int MAX_VISIBLE_OPTIONS = 3;
	constexpr float PANEL_HORIZONTAL_MARGIN = 160.0f;
	constexpr float PANEL_WIDTH_RATIO = 0.70f;
	constexpr float PANEL_MIN_WIDTH = 880.0f;
	constexpr float PANEL_MAX_WIDTH = 1220.0f;
	constexpr float PANEL_HEIGHT_RATIO = 0.44f;
	constexpr float PANEL_MIN_HEIGHT = 400.0f;
	constexpr float PANEL_MAX_HEIGHT = 500.0f;
	constexpr float HINT_SCALE = 0.50f;
}

void LevelUpUI_Initialize()
{
	UIDraw_Initialize();
}

void LevelUpUI_Finalize()
{
}

void LevelUpUI_Draw(
	const std::vector<SkillDefinition>& options,
	int selectedOptionIndex)
{
	if (options.empty())
	{
		return;
	}

	const float screenW =
		static_cast<float>(Direct3D_GetBackBufferWidth());
	const float screenH =
		static_cast<float>(Direct3D_GetBackBufferHeight());

	UIDraw_FilledRect(0.0f, 0.0f, screenW, screenH, UITheme::Overlay);

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

	UIDraw_Panel(panelX, panelY, panelW, panelH);

	const wchar_t* titleText = L"LEVEL UP";
	UIDraw_TextCentered(
		titleText,
		panelX,
		panelY + panelH * 0.07f,
		panelW,
		UITheme::TitleScale,
		UITheme::Primary);

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

		UIDraw_FilledRect(
			cardX,
			cardY,
			cardW,
			cardH,
			selected ? UITheme::CardSelected : UITheme::Card);
		UIDraw_BorderRect(
			cardX,
			cardY,
			cardW,
			cardH,
			selected ? 5.0f : 2.0f,
			selected ? UITheme::Primary : UITheme::GetRarityColor(options[i].rarity));

		const SkillDefinition& skill = options[i];
		const DirectX::XMFLOAT4 rarityColor = UITheme::GetRarityColor(skill.rarity);
		const std::wstring rarityLabel =
			L"[" + std::wstring(UITheme::GetRarityLabel(skill.rarity)) + L"]";
		UIFont_Draw(
			rarityLabel.c_str(),
			cardX + 18.0f,
			cardY + 16.0f,
			UITheme::BodyScale,
			rarityColor);

		UIFont_DrawWrapped(
			skill.name.c_str(),
			cardX + 18.0f,
			cardY + 48.0f,
			cardW - 36.0f,
			UITheme::SubtitleScale,
			selected ? UITheme::Primary : rarityColor);

		UIFont_DrawWrapped(
			skill.description.c_str(),
			cardX + 18.0f,
			cardY + 112.0f,
			cardW - 36.0f,
			UITheme::BodyScale,
			UITheme::Description);

		const std::wstring numberLabel =
			L"[" + std::to_wstring(i + 1) + L"]";
		UIFont_Draw(
			numberLabel.c_str(),
			cardX + cardW - 46.0f,
			cardY + cardH - 38.0f,
			UITheme::BodyScale,
			selected ? UITheme::Primary : UITheme::TextMuted);
	}

	const wchar_t* hintText =
		L"Left / A: Select    Right / D: Select    Enter: Confirm";
	UIDraw_TextCentered(
		hintText,
		panelX,
		panelY + panelH - 48.0f,
		panelW,
		HINT_SCALE,
		UITheme::TextMuted);
}
