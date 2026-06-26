#include "title.h"
#include "fade.h"
#include "key_logger.h"
#include "scene.h"
#include "sprite.h"
#include "texture.h"
#include "Font.h"
#include <string>
#include "direct3d.h"
#include "score.h"

// 定义标题画面的子状态
enum TitleMenuState {
	MENU_MAIN,          // 主菜单 (开始游戏 / 设置)
	MENU_SETTINGS,      // 设置菜单
	MENU_INSTRUCTIONS   // 操作说明界面
};

namespace
{
	int g_TitleBG = -1;       // 标题背景图
	int g_TexWhite = -1;      // 纯白图片

	// 状态控制
	TitleMenuState g_CurrentState = MENU_MAIN;
	int g_MainCursor = 0;      // 主菜单光标

	void DrawTitleImageCentered(float screenW, float screenH)
	{
		if (g_TitleBG < 0) return;

		const float texW = static_cast<float>(Texture_GetWidth(g_TitleBG));
		const float texH = static_cast<float>(Texture_GetHeight(g_TitleBG));
		if (texW <= 0.0f || texH <= 0.0f) return;

		// 标题图按原图尺寸绘制，只做居中，不拉伸适配屏幕。
		const float destX = (screenW - texW) * 0.5f;
		const float destY = (screenH - texH) * 0.5f;
		Sprite_Draw(g_TitleBG, destX, destY);
	}

	void DrawSolidBackground(float screenW, float screenH, const DirectX::XMFLOAT4& color)
	{
		if (g_TexWhite < 0) return;
		Sprite_Draw(g_TexWhite, 0.0f, 0.0f, screenW, screenH, color);
	}
}

void Title_Initialize()
{
	Score_Reset();
	g_TitleBG = Texture_LoadFromFile(L"resource/texture/Title.png");
	g_TexWhite = Texture_LoadFromFile(L"resource/texture/white.png");

	g_CurrentState = MENU_MAIN;
	g_MainCursor = 0;
}

void Title_Finalize()
{
	Texture_Release(g_TitleBG);
	Texture_Release(g_TexWhite);
}

void Title_Update(double)
{
	if (Fade_GetState() != FADE_STATE_NONE) {
		if (Fade_GetState() == FADE_STATE_FINISHED_OUT) {
			// 【修改】去 Loading 界面，而不是直接去 Game
			Scene_Change(SCENE_LOADING);
		}
		return;
	}

	// ====================================================
	// 状态 A: 主菜单
	// ====================================================
	if (g_CurrentState == MENU_MAIN)
	{
		if (KeyLogger_IsTrigger(KK_UP))   g_MainCursor--;
		if (KeyLogger_IsTrigger(KK_DOWN)) g_MainCursor++;

		if (g_MainCursor < 0) g_MainCursor = 1;
		if (g_MainCursor > 1) g_MainCursor = 0;

		if (KeyLogger_IsTrigger(KK_ENTER))
		{
			if (g_MainCursor == 0) {
				g_CurrentState = MENU_INSTRUCTIONS;
			}
			else if (g_MainCursor == 1) {
				g_CurrentState = MENU_SETTINGS;
			}
		}
	}
	// ====================================================
	// 状态 B: 设置菜单
	// ====================================================
	else if (g_CurrentState == MENU_SETTINGS)
	{
		// 当前设置页没有可调选项，只处理返回。
		if (KeyLogger_IsTrigger(KK_ENTER) || KeyLogger_IsTrigger(KK_ESCAPE)) {
			g_CurrentState = MENU_MAIN;
		}
	}
	// ====================================================
	// 状态 C: 操作说明界面
	// ====================================================
	else if (g_CurrentState == MENU_INSTRUCTIONS)
	{
		if (KeyLogger_IsTrigger(KK_ENTER))
		{
			Fade_Start(2.0, true);
		}
		else if (KeyLogger_IsTrigger(KK_ESCAPE))
		{
			g_CurrentState = MENU_MAIN;
		}
	}
}

void Title_Draw()
{
	float screenW = (float)Direct3D_GetBackBufferWidth();
	float screenH = (float)Direct3D_GetBackBufferHeight();

	Direct3D_SetDepthEnable(false);

	DirectX::XMFLOAT4 colSelected = { 1.0f, 1.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT4 colNormal = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 colGray = { 0.5f, 0.5f, 0.5f, 1.0f };
	DirectX::XMFLOAT4 colRed = { 1.0f, 0.2f, 0.2f, 1.0f };

	// ====================================================
	// 绘制: 主菜单
	// ====================================================
	if (g_CurrentState == MENU_MAIN)
	{
		DrawTitleImageCentered(screenW, screenH);

		float menuX = (screenW / 2.0f) - 100.0f;

		// 【优化】使用屏幕高度的百分比，而不是固定像素
		// 55% 处开始画
		float startY = screenH * 0.55f;
		float stepY = screenH * 0.1f;   // 间距 10%

		Font_Draw(L"START GAME", menuX, startY, (g_MainCursor == 0) ? colSelected : colNormal);
		Font_Draw(L"SETTINGS", menuX, startY + stepY, (g_MainCursor == 1) ? colSelected : colNormal);
	}
	// ====================================================
	// 绘制: 设置菜单
	// ====================================================
	else if (g_CurrentState == MENU_SETTINGS)
	{
		DrawTitleImageCentered(screenW, screenH);

		float baseX = (screenW / 2.0f) - 200.0f;
		float baseY = 400.0f;
		// 提示信息
		Font_Draw(L"SETTINGS UNAVAILABLE", baseX + 50, baseY + 100, colGray);

		// Back
		baseY += 150.0f;
		// 无论光标在哪，都显示为选中状态，因为只能返回
		Font_Draw(L"BACK TO TITLE", (screenW / 2.0f) - 120.0f, baseY + 100, colSelected);
	}
	// ====================================================
	// 绘制: 操作说明界面
	// ====================================================
	else if (g_CurrentState == MENU_INSTRUCTIONS)
	{
		DrawSolidBackground(screenW, screenH, { 0.0f, 0.0f, 0.0f, 1.0f });

		Font_Draw(L"HOW TO PLAY", (screenW / 2.0f) - 100.0f, 150, colSelected);

		float textX = (screenW / 2.0f) - 300.0f;
		float textY = 250.0f;
		float spacing = 60.0f;

		Font_Draw(L"W A S D       :   MOVE CHARACTER", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"MOUSE         :   AIM / ROTATE", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"LEFT CLICK    :   SHOOT", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"R KEY         :   RELOAD WEAPON", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"F KEY         :   MELEE ATTACK", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"I KEY         :   OPEN BAG", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"ENTER KEY     :   USE ITEM", textX, textY, colNormal); textY += spacing;

		textY += 40.0f;
		Font_Draw(L"OBJECTIVE     :   SURVIVE AND DEFEAT ENEMIES", textX, textY, colRed);

		Font_Draw(L"PRESS [ENTER] TO START MISSION", (screenW / 2.0f) - 250.0f, 800, { 1.0f, 1.0f, 0.0f, (float)(abs(sin(GetTickCount() * 0.005))) });
	}
	Direct3D_SetDepthEnable(true);
}
