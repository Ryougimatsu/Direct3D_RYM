#include "score.h"
#include <algorithm>
#include "sprite.h"
#include "texture.h"
#include "Font.h"
#include <iomanip>
#include <sstream>
#include <string>
// 确保这里的字体大小和你图片里单个数字的大小一致
static constexpr float SCORE_FONT_SIZE = 32.0f;

namespace
{
	unsigned int g_Score = 0;       // 实际分数
	unsigned int g_ViewScore = 0;   // 滚动显示的分数
	unsigned int g_CounterStop = 1; // 上限
	int g_Digit = 1;                // 位数
	float g_X = 0.0f, g_Y = 0.0f;  // 坐标
	double g_SurvivalTime = 0.0;
}


void Score_Initialize(float x, float y, int digit)
{
	g_ViewScore = 0;
	g_Digit = digit;
	g_Score = 0;
	g_X = x;
	g_Y = y;

	// 计算最大显示值 (例如 6位 就是 999999)
	g_CounterStop = 1;
	for (int i = 0; i < digit; ++i)
	{
		g_CounterStop *= 10;
	}
	g_CounterStop--;

}

void Score_Finalize()
{
	// 可以在这里释放纹理，但通常建议在 main 或 Texture_AllRelease 中统一释放
}

void Score_Draw()
{
	unsigned int displayValue = std::min(g_ViewScore, g_CounterStop);

	// 2. 格式化字符串 (例如: 100 -> "000100")
	std::wstringstream wss;
	wss << std::setw(g_Digit) << std::setfill(L'0') << displayValue;
	std::wstring scoreText = wss.str();

	// 3. 调用字体绘制
	// 参数：文本, X坐标, Y坐标, 颜色(白色)
	Font_Draw(scoreText.c_str(), g_X, g_Y, { 1.0f, 1.0f, 1.0f, 1.0f });
}

void Score_Update()
{
	if (g_ViewScore < g_Score) {
		// 简单的追赶算法
		long long diff = (long long)g_Score - g_ViewScore;

		if (diff > 10000) g_ViewScore += 1000;
		else if (diff > 1000) g_ViewScore += 100;
		else if (diff > 100) g_ViewScore += 10;
		else g_ViewScore++;
	}
	else {
		g_ViewScore = g_Score;
	}
}

unsigned int Score_GetScore()
{
	return g_Score;
}

void Score_AddScore(int score)
{
	g_Score += score;
}

void Score_Reset()
{
	g_Score = 0;
	g_ViewScore = 0;
}

void Score_SetPosition(float x, float y)
{
	g_X = x;
	g_Y = y;
}

void Score_SetTime(double time)
{
	g_SurvivalTime = time;
}

double Score_GetTime()
{
	return g_SurvivalTime;
}
