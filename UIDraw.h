#pragma once

#include <DirectXMath.h>

void UIDraw_Initialize();
void UIDraw_Finalize();

void UIDraw_FilledRect(
	float x,
	float y,
	float width,
	float height,
	const DirectX::XMFLOAT4& color);

void UIDraw_BorderRect(
	float x,
	float y,
	float width,
	float height,
	float thickness,
	const DirectX::XMFLOAT4& color);

void UIDraw_Panel(
	float x,
	float y,
	float width,
	float height);

void UIDraw_Panel(
	float x,
	float y,
	float width,
	float height,
	const DirectX::XMFLOAT4& fillColor,
	const DirectX::XMFLOAT4& borderColor,
	float borderThickness);

void UIDraw_TextCentered(
	const wchar_t* text,
	float x,
	float y,
	float width,
	float scale,
	const DirectX::XMFLOAT4& color);
