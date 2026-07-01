#pragma once

#include <DirectXMath.h>

#include "SkillDefinition.h"

namespace UITheme
{
	inline const DirectX::XMFLOAT4 Overlay{ 0.0f, 0.0f, 0.0f, 0.72f };
	inline const DirectX::XMFLOAT4 Panel{ 0.035f, 0.045f, 0.075f, 0.90f };
	inline const DirectX::XMFLOAT4 PanelStrong{ 0.045f, 0.060f, 0.105f, 0.96f };
	inline const DirectX::XMFLOAT4 Card{ 0.100f, 0.120f, 0.180f, 0.94f };
	inline const DirectX::XMFLOAT4 CardSelected{ 0.175f, 0.215f, 0.330f, 0.98f };
	inline const DirectX::XMFLOAT4 Border{ 0.250f, 0.340f, 0.540f, 0.95f };
	inline const DirectX::XMFLOAT4 Primary{ 1.000f, 0.860f, 0.220f, 1.00f };
	inline const DirectX::XMFLOAT4 Text{ 0.900f, 0.950f, 1.000f, 1.00f };
	inline const DirectX::XMFLOAT4 TextMuted{ 0.660f, 0.750f, 0.920f, 1.00f };
	inline const DirectX::XMFLOAT4 Description{ 0.720f, 0.800f, 0.920f, 1.00f };
	inline const DirectX::XMFLOAT4 Success{ 0.240f, 0.920f, 0.500f, 1.00f };
	inline const DirectX::XMFLOAT4 Danger{ 1.000f, 0.160f, 0.160f, 1.00f };
	inline const DirectX::XMFLOAT4 Disabled{ 0.520f, 0.560f, 0.640f, 1.00f };

	inline constexpr float TitleScale = 0.82f;
	inline constexpr float SubtitleScale = 0.58f;
	inline constexpr float BodyScale = 0.52f;
	inline constexpr float DebugScale = 0.42f;

	inline DirectX::XMFLOAT4 WithAlpha(
		const DirectX::XMFLOAT4& color,
		float alpha)
	{
		return { color.x, color.y, color.z, alpha };
	}

	inline const wchar_t* GetRarityLabel(SkillRarity rarity)
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
			return L"LEGENDARY";
		default:
			return L"COMMON";
		}
	}

	inline DirectX::XMFLOAT4 GetRarityColor(SkillRarity rarity)
	{
		switch (rarity)
		{
		case SkillRarity::Common:
			return { 0.860f, 0.880f, 0.920f, 1.00f };
		case SkillRarity::Rare:
			return { 0.250f, 0.550f, 1.000f, 1.00f };
		case SkillRarity::Epic:
			return { 0.720f, 0.320f, 1.000f, 1.00f };
		case SkillRarity::Legendary:
			return { 1.000f, 0.720f, 0.180f, 1.00f };
		default:
			return Text;
		}
	}
}
