#include "loading.h"
#include "scene.h"
#include "game.h"      
#include "Font.h"      
#include "direct3d.h"  
#include <thread>      
#include <atomic>      
#include <string>
#include "sprite.h"
#include "texture.h"

namespace
{
	std::thread g_LoaderThread;       // 加载线程
	std::atomic<bool> g_IsFinished;   // 标记是否加载完成
	float g_Timer = 0.0f;             // 用于文字闪烁动画
	int g_LoadingBG = -1;
}

void Loading_Initialize()
{
	g_IsFinished = false;
	g_Timer = 0.0f;
	g_LoadingBG = Texture_LoadFromFile(L"resource/texture/Loading.png");
	// 启动一个新线程，去执行 Game_LoadContent
	g_LoaderThread = std::thread([]() {
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		Game_LoadContent(); // 执行繁重的加载任务
		g_IsFinished = true; // 任务做完了，举手示意
		});
}

void Loading_Finalize()
{
	// 确保线程已经汇合（Join），防止报错
	if (g_LoaderThread.joinable()) {
		g_LoaderThread.join();
	}
	Texture_Release(g_LoadingBG);
}

void Loading_Update(double elapsed_time)
{
	g_Timer += (float)elapsed_time;

	// 如果线程报告说它干完活了
	if (g_IsFinished)
	{
		// 切换到游戏场景
		Scene_Change(SCENE_GAME);
	}
}

void Loading_Draw()
{
	Direct3D_ClearBackBuffer();
	Sprite_Begin();
	float screenW = (float)Direct3D_GetBackBufferWidth();
	float screenH = (float)Direct3D_GetBackBufferHeight();

	Sprite_Draw(g_LoadingBG, 0.0f, 0.0f, screenW, screenH, { 1.0f, 1.0f, 1.0f, 1.0f });
	Sprite_End();
}