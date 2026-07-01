#include "result.h"
#include "score.h"
#include "key_logger.h"
#include "scene.h"
#include "direct3d.h"
#include "fade.h"
#include "texture.h"
#include "sprite.h"
#include "Font.h"
#include "UITheme.h"
#include <string>
#include <sstream>
#include <iomanip>      // 用于控制小数位数
#include <cmath>        // 用于闪烁动画的 sin 函数
#include <algorithm>
#include <cwchar>

namespace
{
	int g_ResultBGTexID = -1; // 背景图纹理ID
	ResultOutcome g_ResultOutcome = ResultOutcome::MissionComplete;

	float EstimateFontTextWidth(const wchar_t* text)
	{
		if (!text)
		{
			return 0.0f;
		}

		// Font_Draw 使用位图字体且没有公开测量接口，这里用平均字符宽度
		// 做居中估算，避免固定像素在不同窗口尺寸下偏移太明显。
		return static_cast<float>(std::wcslen(text)) * 24.0f;
	}

	void DrawFontCentered(
		const wchar_t* text,
		float screenW,
		float y,
		const DirectX::XMFLOAT4& color)
	{
		Font_Draw(
			text,
			(screenW - EstimateFontTextWidth(text)) * 0.5f,
			y,
			color);
	}
}

void Result_SetOutcome(ResultOutcome outcome)
{
	g_ResultOutcome = outcome;
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
	const float titleY = screenH * 0.18f;
	const float statX = screenW * 0.5f - 280.0f;
	const float statY = screenH * 0.40f;
	const float statLineGap = std::clamp(screenH * 0.075f, 64.0f, 84.0f);
	const float hintY = screenH * 0.78f;

	// --- 1. 绘制背景图 (最底层) ---
	Direct3D_SetDepthEnable(false); 
	if (g_ResultBGTexID != -1)
	{
		// 拉伸背景图铺满屏幕
		Sprite_Draw(g_ResultBGTexID, 0.0f, 0.0f, screenW, screenH, 
			0, 0, Texture_GetWidth(g_ResultBGTexID), Texture_GetHeight(g_ResultBGTexID));
	}

	// --- 2. 绘制结果标题 ---
	if (g_ResultOutcome == ResultOutcome::MissionComplete)
	{
		DrawFontCentered(
			L"MISSION COMPLETE",
			screenW,
			titleY,
			UITheme::Primary);
	}
	else
	{
		DrawFontCentered(
			L"GAME OVER",
			screenW,
			titleY,
			UITheme::Danger);
	}

	// --- 3. 绘制统计信息 ---
	std::wstringstream wss;

	// (A) 生存时间 (依赖于 score 模块的扩展)
	// 如果您还没在 score.h 中加 Score_GetTime()，请暂时注释掉下面这几行
	double time = Score_GetTime();
	wss << L"SURVIVAL TIME :  " << std::fixed << std::setprecision(2) << time << L" s";
	Font_Draw(
		wss.str().c_str(),
		statX,
		statY,
		UITheme::Text);

	// (B) 总分数
	wss.str(L""); // 清空流
	wss << L"TOTAL SCORE   :  " << Score_GetScore();
	Font_Draw(
		wss.str().c_str(),
		statX,
		statY + statLineGap,
		UITheme::Text);

	// --- 4. 绘制底部提示 (闪烁效果) ---
	// 利用系统时间制作呼吸灯效果
	static float timer = 0.0f;
	timer += 0.05f; // 简单的计时模拟，如果有 elapsed_time 更好，这里用静态变量凑合
	float alpha = 0.35f + fabsf(sinf(timer)) * 0.65f; // 0.35 ~ 1.0 循环

	DrawFontCentered(
		L"PRESS [ENTER] TO TITLE",
		screenW,
		hintY,
		UITheme::WithAlpha(UITheme::Success, alpha));

	Direct3D_SetDepthEnable(true); // 恢复深度测试
}
