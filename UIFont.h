#pragma once

#include <DirectXMath.h>

void UIFont_Initialize();
void UIFont_Finalize();

float UIFont_MeasureText(const wchar_t* text, float scale = 1.0f);

void UIFont_Draw(
	const wchar_t* text,
	float x,
	float y,
	float scale,
	const DirectX::XMFLOAT4& color);

void UIFont_DrawWrapped(
	const wchar_t* text,
	float x,
	float y,
	float maxWidth,
	float scale,
	const DirectX::XMFLOAT4& color);
