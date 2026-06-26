#include "UIFont.h"

#include "sprite.h"
#include "texture.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace
{
	struct CharInfo
	{
		int srcX = 0;
		int srcY = 0;
		int srcW = 0;
		int srcH = 0;
		int xoffset = 0;
		int yoffset = 0;
		int xadvance = 0;
	};

	int g_FontTextureID = -1;
	std::map<wchar_t, CharInfo> g_CharMap;

	constexpr float DEFAULT_LINE_HEIGHT = 48.0f;
	constexpr float FALLBACK_ADVANCE = 16.0f;

	void ParseCharLine(const std::string& line)
	{
		std::stringstream stream(line);
		std::string token;
		int charId = 0;
		CharInfo info;

		while (stream >> token)
		{
			const std::size_t pos = token.find('=');
			if (pos == std::string::npos)
			{
				continue;
			}

			const std::string key = token.substr(0, pos);
			const int value = std::stoi(token.substr(pos + 1));

			if (key == "id") charId = value;
			else if (key == "x") info.srcX = value;
			else if (key == "y") info.srcY = value;
			else if (key == "width") info.srcW = value;
			else if (key == "height") info.srcH = value;
			else if (key == "xoffset") info.xoffset = value;
			else if (key == "yoffset") info.yoffset = value;
			else if (key == "xadvance") info.xadvance = value;
		}

		if (charId != 0 && info.srcW > 0 && info.srcH > 0)
		{
			g_CharMap[static_cast<wchar_t>(charId)] = info;
		}
	}

	float GetCharAdvance(wchar_t c)
	{
		const auto it = g_CharMap.find(c);
		if (it != g_CharMap.end())
		{
			return static_cast<float>(it->second.xadvance);
		}

		return FALLBACK_ADVANCE;
	}

	bool CanDraw()
	{
		return g_FontTextureID >= 0 && !g_CharMap.empty();
	}
}

void UIFont_Initialize()
{
	g_FontTextureID = Texture_LoadFromFile(L"resource/texture/font.png");
	g_CharMap.clear();

	std::ifstream file("resource/texture/font.fnt");
	if (!file.is_open())
	{
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.rfind("char ", 0) == 0)
		{
			ParseCharLine(line);
		}
	}
}

void UIFont_Finalize()
{
	g_FontTextureID = -1;
	g_CharMap.clear();
}

float UIFont_MeasureText(const wchar_t* text, float scale)
{
	if (!text || scale <= 0.0f)
	{
		return 0.0f;
	}

	float currentLineWidth = 0.0f;
	float maxLineWidth = 0.0f;
	for (int i = 0; text[i] != L'\0'; ++i)
	{
		if (text[i] == L'\n')
		{
			maxLineWidth = std::max(maxLineWidth, currentLineWidth);
			currentLineWidth = 0.0f;
			continue;
		}

		currentLineWidth += GetCharAdvance(text[i]) * scale;
	}

	return std::max(maxLineWidth, currentLineWidth);
}

void UIFont_Draw(
	const wchar_t* text,
	float x,
	float y,
	float scale,
	const DirectX::XMFLOAT4& color)
{
	if (!CanDraw() || !text || scale <= 0.0f)
	{
		return;
	}

	float cursorX = x;
	float cursorY = y;
	const float lineHeight = DEFAULT_LINE_HEIGHT * scale;

	for (int i = 0; text[i] != L'\0'; ++i)
	{
		const wchar_t c = text[i];
		if (c == L'\n')
		{
			cursorX = x;
			cursorY += lineHeight;
			continue;
		}

		const auto it = g_CharMap.find(c);
		if (it == g_CharMap.end())
		{
			cursorX += FALLBACK_ADVANCE * scale;
			continue;
		}

		const CharInfo& info = it->second;
		Sprite_Draw(
			g_FontTextureID,
			cursorX + info.xoffset * scale,
			cursorY + info.yoffset * scale,
			info.srcW * scale,
			info.srcH * scale,
			info.srcX,
			info.srcY,
			info.srcW,
			info.srcH,
			color);

		cursorX += info.xadvance * scale;
	}
}

void UIFont_DrawWrapped(
	const wchar_t* text,
	float x,
	float y,
	float maxWidth,
	float scale,
	const DirectX::XMFLOAT4& color)
{
	if (!CanDraw() || !text || scale <= 0.0f || maxWidth <= 0.0f)
	{
		return;
	}

	std::wstring remainingText(text);
	float cursorY = y;
	const float lineHeight = DEFAULT_LINE_HEIGHT * scale;

	while (!remainingText.empty())
	{
		if (remainingText.front() == L'\n')
		{
			remainingText.erase(0, 1);
			cursorY += lineHeight;
			continue;
		}

		const std::size_t newlinePos = remainingText.find(L'\n');
		const std::wstring currentParagraph =
			newlinePos == std::wstring::npos
				? remainingText
				: remainingText.substr(0, newlinePos);

		std::size_t breakPos = std::wstring::npos;
		float currentLineWidth = 0.0f;
		for (std::size_t i = 0; i < currentParagraph.length(); ++i)
		{
			currentLineWidth += GetCharAdvance(currentParagraph[i]) * scale;
			if (currentLineWidth > maxWidth)
			{
				breakPos = i;
				break;
			}
		}

		if (breakPos != std::wstring::npos)
		{
			const std::size_t wordBreakPos =
				currentParagraph.find_last_of(L" \t,.", breakPos);
			if (wordBreakPos != std::wstring::npos && wordBreakPos > 0)
			{
				breakPos = wordBreakPos;
			}
			else if (breakPos == 0)
			{
				breakPos = 1;
			}

			const std::wstring lineToDraw =
				currentParagraph.substr(0, breakPos);
			UIFont_Draw(lineToDraw.c_str(), x, cursorY, scale, color);
			remainingText = remainingText.substr(breakPos);
			while (!remainingText.empty() &&
				(remainingText.front() == L' ' ||
					remainingText.front() == L'\t'))
			{
				remainingText.erase(0, 1);
			}
		}
		else
		{
			UIFont_Draw(currentParagraph.c_str(), x, cursorY, scale, color);
			remainingText =
				newlinePos == std::wstring::npos
					? L""
					: remainingText.substr(newlinePos);
		}

		cursorY += lineHeight;
	}
}
