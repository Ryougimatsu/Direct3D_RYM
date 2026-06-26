#include "result.h"
#include "score.h"
#include "key_logger.h"
#include "scene.h"
#include "direct3d.h"
#include "fade.h"
#include "texture.h"
#include "sprite.h"
#include "Font.h"       // 引入字体模块
#include <string>
#include <sstream>
#include <iomanip>      // 用于控制小数位数
#include <cmath>        // 用于闪烁动画的 sin 函数

namespace
{
	int g_ResultBGTexID = -1; // 背景图纹理ID
}

void Result_Initialize()
{
	// 1. 加载背景图片
	g_ResultBGTexID = Texture_LoadFromFile(L"resource/texture/RESULT.png");

	// 2. 开始淡入效果 (黑色 -> 亮起)
	Fade_Start(0.5, false, { 0.0f, 0.0f, 0.0f });

	// 注意：我们将在 Result_Draw 中手动绘制分数文字，
	// 所以这里不需要像之前那样调用 Score_SetPosition 了。
}

void Result_Finalize()
{
	// 释放背景图片
	if (g_ResultBGTexID != -1)
	{
		Texture_Release(g_ResultBGTexID);
		g_ResultBGTexID = -1;
	}
}

void Result_Update(double elapsed_time)
{
	(void)elapsed_time;
	// 1. 如果正在淡入/淡出，不允许操作
	if (Fade_GetState() != FADE_STATE_NONE) return;

	// 2. 按下回车键返回标题
	if (KeyLogger_IsTrigger(KK_ENTER))
	{
		// 触发淡出效果，完成后切换场景（在 Scene_Update 中处理，或者在这里直接切）
		// 为了简单起见，这里直接切换，或者您可以加一个淡出逻辑
		Scene_Change(SCENE_TITLE);
	}
}

void Result_Draw()
{
	float screenW = (float)Direct3D_GetBackBufferWidth();
	float screenH = (float)Direct3D_GetBackBufferHeight();
	float centerX = screenW / 2.0f;

	// --- 1. 绘制背景图 (最底层) ---
	Direct3D_SetDepthEnable(false); 
	if (g_ResultBGTexID != -1)
	{
		// 拉伸背景图铺满屏幕
		Sprite_Draw(g_ResultBGTexID, 0.0f, 0.0f, screenW, screenH, 
			0, 0, Texture_GetWidth(g_ResultBGTexID), Texture_GetHeight(g_ResultBGTexID));
	}

	// --- 2. 绘制标题 "RESULT" ---
	Font_Draw(L"MISSION COMPLETE", centerX - 200.0f, 100.0f, { 1.0f, 1.0f, 0.0f, 1.0f }); // 黄色标题

	// --- 3. 绘制统计信息 ---
	std::wstringstream wss;

	// (A) 生存时间 (依赖于 score 模块的扩展)
	// 如果您还没在 score.h 中加 Score_GetTime()，请暂时注释掉下面这几行
	double time = Score_GetTime();
	wss << L"SURVIVAL TIME :  " << std::fixed << std::setprecision(2) << time << L" s";
	Font_Draw(wss.str().c_str(), centerX - 250.0f, 300.0f, { 1.0f, 1.0f, 1.0f, 1.0f }); // 白色

	// (B) 总分数
	wss.str(L""); // 清空流
	wss << L"TOTAL SCORE   :  " << Score_GetScore();
	Font_Draw(wss.str().c_str(), centerX - 250.0f, 380.0f, { 1.0f, 1.0f, 1.0f, 1.0f }); // 白色

	// --- 4. 绘制底部提示 (闪烁效果) ---
	// 利用系统时间制作呼吸灯效果
	static float timer = 0.0f;
	timer += 0.05f; // 简单的计时模拟，如果有 elapsed_time 更好，这里用静态变量凑合
	float alpha = fabsf(sinf(timer)); // 0.0 ~ 1.0 循环

	Font_Draw(L"PRESS [ENTER] TO TITLE", centerX - 220.0f, screenH - 150.0f, { 0.0f, 1.0f, 0.0f, alpha }); // 绿色闪烁

	Direct3D_SetDepthEnable(true); // 恢复深度测试
}
