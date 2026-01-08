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
	MENU_INSTRUCTIONS   // 【新增】操作说明界面
};

namespace
{
	int g_TitleBG = -1;       // 标题背景图
	int g_TitleLogo = -1;     // Logo
	int g_TexWhite = -1;      // 纯白图片
	int g_InstructionBG = -1; // 【新增】操作说明背景图

	// 状态控制
	TitleMenuState g_CurrentState = MENU_MAIN;
	int g_MainCursor = 0;      // 主菜单光标
	int g_SettingsCursor = 0;  // 设置光标

	// 音量数据
	float g_VolumeBGM = 1.0f;
	float g_VolumeSE = 1.0f;
}

void Title_Initialize()
{
	Score_Reset();
	g_TitleBG = Texture_LoadFromFile(L"resource/texture/Title.png");
	g_TitleLogo = Texture_LoadFromFile(L"resource/texture/Title.png");
	g_TexWhite = Texture_LoadFromFile(L"resource/texture/white.png");

	// 【新增】加载操作说明背景图
	// 请确保有一张图叫 instructions.png，或者暂时用 bg_v.png 代替
	g_InstructionBG = Texture_LoadFromFile(L"resource/texture/Title.png");
	if (g_InstructionBG == -1) {
		g_InstructionBG = g_TitleBG; // 如果没找到，就复用标题背景
	}


	g_CurrentState = MENU_MAIN;
	g_MainCursor = 0;
}

void Title_Finalize()
{
	Texture_Release(g_TitleBG);
	Texture_Release(g_TitleLogo);
	Texture_Release(g_TexWhite);
	Texture_Release(g_InstructionBG); // 【新增】释放资源
}

void Title_Update(double elapsed_time)
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
				g_SettingsCursor = 2; // 【修改】默认光标直接放在 BACK 上，因为上面两个被禁用了
			}
		}
	}
	// ====================================================
	// 状态 B: 设置菜单
	// ====================================================
	else if (g_CurrentState == MENU_SETTINGS)
	{
		/* 【注释】暂时禁用音量调节逻辑
		if (KeyLogger_IsTrigger(KK_UP))   g_SettingsCursor--;
		if (KeyLogger_IsTrigger(KK_DOWN)) g_SettingsCursor++;
		if (g_SettingsCursor < 0) g_SettingsCursor = 2;
		if (g_SettingsCursor > 2) g_SettingsCursor = 0;

		float step = 0.05f;
		if (g_SettingsCursor == 0) { // BGM
			if (KeyLogger_IsTrigger(KK_LEFT))  GameSettings::VolumeBGM -= step;
			if (KeyLogger_IsTrigger(KK_RIGHT)) GameSettings::VolumeBGM += step;
		}
		else if (g_SettingsCursor == 1) { // SE
			if (KeyLogger_IsTrigger(KK_LEFT))  GameSettings::VolumeSE -= step;
			if (KeyLogger_IsTrigger(KK_RIGHT)) GameSettings::VolumeSE += step;
		}

		if (GameSettings::VolumeBGM < 0.0f) GameSettings::VolumeBGM = 0.0f; if (GameSettings::VolumeBGM > 1.0f) GameSettings::VolumeBGM = 1.0f;
		if (GameSettings::VolumeSE < 0.0f)  GameSettings::VolumeSE = 0.0f;  if (GameSettings::VolumeSE > 1.0f)  GameSettings::VolumeSE = 1.0f;
		*/

		// 【新增】只检测退出键 (因为没有选项可选了)
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

	float texW = 1024.0f;
	float texH = 1536.0f;

	float destW, destH, destX, destY;

	float screenAspectRatio = screenW / screenH;
	float textureAspectRatio = texW / texH;

	if (textureAspectRatio > screenAspectRatio)
	{
		// 情况 A：图片比屏幕更“扁/宽”（例如横屏电影在方屏上看）
		// 以屏幕宽度为基准，上下留黑边
		destW = screenW;
		destH = screenW / textureAspectRatio;
		destX = 0.0f;
		destY = (screenH - destH) / 2.0f; // 垂直居中计算
	}
	else
	{
		// 情况 B：图片比屏幕更“高/窄”（这是你目前的情况，海报是竖的，屏幕是横的）
		// 以屏幕高度为基准，左右留黑边
		destH = screenH;
		destW = screenH * textureAspectRatio;
		destX = (screenW - destW) / 2.0f; // 水平居中计算
		destY = 0.0f;
	}
	Sprite_Draw(g_TitleBG, destX, destY, destW, destH, 0, 0, texW, texH);

	Direct3D_SetDepthEnable(false);
	/*float logoW = 540.0f;
	float logoH = 150.0f;
	float logoX = (screenW - logoW) / 2.0f;
	float logoY = 100.0f;
	Sprite_Draw(g_TitleLogo, logoX, logoY, logoW, logoH, 0, 0, 1514, 123);*/

	DirectX::XMFLOAT4 colSelected = { 1.0f, 1.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT4 colNormal = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 colGray = { 0.5f, 0.5f, 0.5f, 1.0f };
	DirectX::XMFLOAT4 colRed = { 1.0f, 0.2f, 0.2f, 1.0f };

	// ====================================================
	// 绘制: 主菜单
	// ====================================================
	if (g_CurrentState == MENU_MAIN)
	{
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
		float baseX = (screenW / 2.0f) - 200.0f;
		float baseY = 400.0f;
		float barW = 400.0f;
		float barH = 24.0f;

		// 提示信息
		Font_Draw(L"SETTINGS UNAVAILABLE", baseX + 50, baseY + 100, colGray);

		/* 【注释】暂时隐藏音量条绘制
		// BGM
		Font_Draw(L"BGM VOLUME", baseX, baseY, (g_SettingsCursor == 0) ? colSelected : colNormal);
		int bgmPercent = (int)(GameSettings::VolumeBGM * 100);
		std::wstring bgmText = std::to_wstring(bgmPercent) + L"%";
		Font_Draw(bgmText.c_str(), baseX + 550, baseY, (g_SettingsCursor == 0) ? colSelected : colNormal);
		Sprite_Draw(g_TexWhite, baseX, baseY + 40, barW, barH, colGray);
		Sprite_Draw(g_TexWhite, baseX, baseY + 40, barW * GameSettings::VolumeBGM, barH, colSelected);

		// SE
		baseY += 120.0f;
		Font_Draw(L"SE VOLUME", baseX, baseY, (g_SettingsCursor == 1) ? colSelected : colNormal);
		int sePercent = (int)(GameSettings::VolumeSE * 100);
		std::wstring seText = std::to_wstring(sePercent) + L"%";
		Font_Draw(seText.c_str(), baseX + 550, baseY, (g_SettingsCursor == 1) ? colSelected : colNormal);
		Sprite_Draw(g_TexWhite, baseX, baseY + 40, barW, barH, colGray);
		Sprite_Draw(g_TexWhite, baseX, baseY + 40, barW * GameSettings::VolumeSE, barH, colSelected);
		*/

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
		Sprite_Draw(g_InstructionBG, 0, 0, screenW, screenH, 1.0f, 1.0f, 1.0f, 1.0f);
		Sprite_Draw(g_TexWhite, 0, 0, screenW, screenH, { 0.0f, 0.0f, 0.0f, 0.7f });

		Font_Draw(L"HOW TO PLAY", (screenW / 2.0f) - 100.0f, 150, colSelected);

		float textX = (screenW / 2.0f) - 300.0f;
		float textY = 250.0f;
		float spacing = 60.0f;

		Font_Draw(L"W A S D       :   MOVE CHARACTER", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"MOUSE         :   AIM / ROTATE", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"LEFT CLICK    :   SHOOT", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"R KEY         :   RELOAD WEAPON", textX, textY, colNormal); textY += spacing;
		Font_Draw(L"F KEY         :   MELEE ATTACK", textX, textY, colNormal); textY += spacing;

		textY += 40.0f;
		Font_Draw(L"OBJECTIVE     :   SURVIVE AND DEFEAT ENEMIES", textX, textY, colRed);

		Font_Draw(L"PRESS [ENTER] TO START MISSION", (screenW / 2.0f) - 250.0f, 800, { 1.0f, 1.0f, 0.0f, (float)(abs(sin(GetTickCount() * 0.005))) });
	}
	Direct3D_SetDepthEnable(true);
}