// gameover.cpp
#include "gameover.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "key_logger.h"
#include "scene.h"
#include "fade.h"

namespace {
	int g_TexGameOver = -1;
}

void GameOver_Initialize()
{
	// 【请确认图片路径】
	g_TexGameOver = Texture_LoadFromFile(L"resource/texture/GAMEOVER.png");
}

void GameOver_Finalize()
{
	Texture_Release(g_TexGameOver);
}

void GameOver_Update(double elapsed_time)
{
	// 按下回车键，返回标题画面
	if (KeyLogger_IsTrigger(KK_ENTER))
	{
		// 也可以直接去 SCENE_GAME (重玩)，看你喜好
		Scene_Change(SCENE_TITLE);
	}
}

void GameOver_Draw()
{
	float w = (float)Direct3D_GetBackBufferWidth();
	float h = (float)Direct3D_GetBackBufferHeight();

	// 绘制全屏背景
	Sprite_Draw(g_TexGameOver, 0.0f, 0.0f, w, h, { 1.0f, 1.0f, 1.0f, 1.0f });
}