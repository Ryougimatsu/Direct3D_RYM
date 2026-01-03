#include "GameUI.h"
#include "sprite.h"
#include "texture.h"
#include "PlayerCharacter.h"


namespace {
	int g_TexWhite = -1; // 用白色图片染色
}

void GameUI_Initialize() {
	g_TexWhite = Texture_LoadFromFile(L"resource/texture/white.png");
}

void GameUI_Draw() {
	if (g_TexWhite == -1) return;
	PlayerCharacter* g_Player = Player_GetInstance();
	// 1. 获取数据
	float hp = g_Player->GetHP();
	float maxHp = 100.0f;
	float ratio = hp / maxHp;

	// 2. 定义位置 (左上角)
	float x = 20.0f;
	float y = 20.0f;
	float w = 200.0f; // 血条总宽
	float h = 20.0f;  // 血条高度

	// 3. 画背景 (黑色)
	Sprite_Draw(g_TexWhite, x - 2, y - 2, w + 4, h + 4, { 0.0f, 0.0f, 0.0f, 1.0f });

	// 4. 画血条 (红色) - 宽度乘以血量比例
	Sprite_Draw(g_TexWhite, x, y, w * ratio, h, { 1.0f, 0.0f, 0.0f, 1.0f });
}