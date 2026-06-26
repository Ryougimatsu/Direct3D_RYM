// gameover.cpp
#include "gameover.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "key_logger.h"
#include "scene.h"
#include "fade.h"
#include "Font.h"
#include <cmath>
namespace {
	int g_TexGameOver = -1;
}

void GameOver_Initialize()
{
	// ����ȷ��ͼƬ·����
	g_TexGameOver = Texture_LoadFromFile(L"resource/texture/GAMEOVER.png");
	Fade_Start(0.5, false, { 0.0f, 0.0f, 0.0f });
}

void GameOver_Finalize()
{
	Texture_Release(g_TexGameOver);
}

void GameOver_Update(double elapsed_time)
{
	(void)elapsed_time;
	// ���»س��������ر��⻭��
	if (KeyLogger_IsTrigger(KK_ENTER))
	{
		// Ҳ����ֱ��ȥ SCENE_GAME (����)������ϲ��
		Scene_Change(SCENE_TITLE);
	}
}

void GameOver_Draw()
{
	float w = (float)Direct3D_GetBackBufferWidth();
	float h = (float)Direct3D_GetBackBufferHeight();

	// ����ȫ������
	Sprite_Draw(g_TexGameOver, 0.0f, 0.0f, w, h, { 1.0f, 1.0f, 1.0f, 1.0f });
	Direct3D_SetDepthEnable(false);
	static float timer = 0.0f;
	timer += 0.05f;
	float alpha = fabsf(sinf(timer)); // ��͸������ 0~1 ֮��ڶ�

	float centerX = w / 2.0f;
	Font_Draw(L"PRESS [ENTER] TO TITLE", centerX - 220.0f, h - 150.0f, { 1.0f, 0.0f, 0.0f, alpha });

	Direct3D_SetDepthEnable(true);
}
