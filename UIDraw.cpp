#include "UIDraw.h"

#include "sprite.h"
#include "texture.h"
#include "UIFont.h"
#include "UITheme.h"

namespace
{
	int g_WhiteTexture = -1;
}

void UIDraw_Initialize()
{
	if (g_WhiteTexture < 0)
	{
		g_WhiteTexture = Texture_LoadFromFile(L"resource/texture/white.png");
	}
}

void UIDraw_Finalize()
{
	if (g_WhiteTexture >= 0)
	{
		Texture_Release(g_WhiteTexture);
		g_WhiteTexture = -1;
	}
}

void UIDraw_FilledRect(
	float x,
	float y,
	float width,
	float height,
	const DirectX::XMFLOAT4& color)
{
	if (g_WhiteTexture < 0)
	{
		UIDraw_Initialize();
	}

	if (g_WhiteTexture >= 0)
	{
		Sprite_Draw(g_WhiteTexture, x, y, width, height, color);
	}
}

void UIDraw_BorderRect(
	float x,
	float y,
	float width,
	float height,
	float thickness,
	const DirectX::XMFLOAT4& color)
{
	UIDraw_FilledRect(x, y, width, thickness, color);
	UIDraw_FilledRect(x, y + height - thickness, width, thickness, color);
	UIDraw_FilledRect(x, y, thickness, height, color);
	UIDraw_FilledRect(x + width - thickness, y, thickness, height, color);
}

void UIDraw_Panel(
	float x,
	float y,
	float width,
	float height)
{
	UIDraw_Panel(
		x,
		y,
		width,
		height,
		UITheme::PanelStrong,
		UITheme::Border,
		3.0f);
}

void UIDraw_Panel(
	float x,
	float y,
	float width,
	float height,
	const DirectX::XMFLOAT4& fillColor,
	const DirectX::XMFLOAT4& borderColor,
	float borderThickness)
{
	UIDraw_FilledRect(x, y, width, height, fillColor);
	UIDraw_FilledRect(x, y, width, 2.0f, UITheme::WithAlpha(UITheme::Primary, 0.55f));
	UIDraw_BorderRect(x, y, width, height, borderThickness, borderColor);
}

void UIDraw_TextCentered(
	const wchar_t* text,
	float x,
	float y,
	float width,
	float scale,
	const DirectX::XMFLOAT4& color)
{
	const float textWidth = UIFont_MeasureText(text, scale);
	UIFont_Draw(text, x + (width - textWidth) * 0.5f, y, scale, color);
}
