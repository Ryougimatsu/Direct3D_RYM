#include "GameUI.h"
#include "sprite.h"
#include "texture.h"
#include "Player.h" // 引用 Player 头文件来获取血量

namespace {
	int g_TexWhite = -1; // 用白色图片染色
}

void GameUI_Initialize() {
	// 加载一张纯白像素图 (或者复用之前的 ui_cursor.png 也可以，只要是纯色的)
	g_TexWhite = Texture_LoadFromFile(L"resource/texture/white.png");
}

void GameUI_Draw() {
	if (g_TexWhite == -1) return;

	// 1. 获取数据
	float hp = Player_GetHP();
	float maxHp = Player_GetMaxHP();
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