#include "game.h"
#include "shader.h"
#include "Sampler.h"
#include "Meshfield.h"
#include "Light.h"
#include <DirectXMath.h>
#include "model.h"
#include "camera.h"
#include "Player_Camera.h"
#include "map.h"
#include "billboard.h"
#include "texture.h"
#include "sprite_anime.h"
#include "bullet.h"
#include "bullet_hit_effect.h"
#include "direct3d.h"
#include "sky.h"
#include "enemy.h"
#include "MapCamera.h"
#include "DebugCamera.h"
#include "mouse.h"
#include "sprite.h"
#include "Inventory.h"
#include "DropItem.h"
#include "GameUI.h"
#include "SkinningModel.h"
#include "SkinningShader.h"
#include <memory>
#include <string>
#include "shader_3d.h"
#include "Skeleton.h"
#include "Animator.h"
#include"PlayerCharacter.h"
#include "enemy_test.h"
#include "Pathfinder.h"
#include "fade.h"
#include "scene.h"
using namespace DirectX;

namespace
{
	bool g_IsDebugCameraMode = false;
	PlayerCharacter* g_Player = nullptr;

}

bool Game_IsLineOfSightBlocked(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end)
{
	return Map_CheckLineOfSightBlocked(start, end);
}

bool Game_CheckCollisionWithWalls(const AABB& objAabb)
{
	return Map_CheckCollision(objAabb);
}

// ------------------------------------------------------------------
// 初始化：仅保留核心环境和你的测试主角
// ------------------------------------------------------------------
void Game_LoadContent()
{
	// 这里放所有耗时的加载函数
	Bullet_Initialize();
	Sky_Initialize();
	Pathfinder::Initialize();
	Map_Initialize();

	Player_Camera_Initialize();
	Inventory_Initialize();
	DropItem_Initialize();

	// 注意：g_Player 的 new 操作也可以放在这里，但要小心全局变量竞争
	// 如果 g_Player 是全局指针，在这里初始化是可以的
	if (!g_Player) { // 防止重复创建
		g_Player = new PlayerCharacter();
		if (!g_Player->Initialize()) {
			OutputDebugStringA("[Game] PlayerCharacter Initialize Failed!\n");
		}
	}

	EnemyTest::LoadAssets(); // 加载敌人资源
	Enemy_Initialize();      // 初始化敌人
	GameUI_Initialize();
}
void Game_Initialize()
{
	// 此时资源已经由 Loading 线程加载完毕了！

	Camera_Initialize();
	DebugCamera_Initialize({ 0.0f, 5.0f, -10.0f }, { 0.0f, 0.0f, 0.0f });

	// 可以在这里重置玩家位置
	if (g_Player) {
		g_Player->SetPosition({ 0.0f, 0.0f, 0.0f });
		// 重置血量等逻辑...
	}

	// 【重要】开始淡入，让画面亮起来
	Fade_Start(1.0, false, { 0.0f, 0.0f, 0.0f });
}

void Game_Update(double elapsed_time)
{
	// 1. 相机模式切换 (Tab 键)
	if (KeyLogger_IsTrigger(KK_TAB))
	{
		g_IsDebugCameraMode = !g_IsDebugCameraMode;
		if (g_IsDebugCameraMode) {
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
			DebugCamera_SetPosition(Player_Camera_GetPosition());
		}
		else {
			Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
		}
	}


	// 先安全地拿一下玩家位置
	DirectX::XMFLOAT3 pPos = { 0.0f, 0.0f, 0.0f };
	if (g_Player) {
		pPos = g_Player->GetPosition();
	}

	// 2. 更新相机位置
	if (g_IsDebugCameraMode) {
		DebugCamera_Update(elapsed_time);
	}
	else {
		Player_Camera_Update(elapsed_time, pPos);
	}
	Enemy_Update(elapsed_time);
	Bullet_Update(elapsed_time);
	Bullet_CheckCollisionWithEnemies();
	Sky_SetPosition(Player_Camera_GetPosition());
	DropItem_Update(elapsed_time);
	Inventory_Update(elapsed_time);
	
	if (!g_IsDebugCameraMode && g_Player)
	{
		g_Player->Update(elapsed_time);


		if (g_Player->IsDeathAnimationFinished())
		{
			delete g_Player;    
			g_Player = nullptr; 
			Scene_Change(SCENE_GAMEOVER);
		}
	}

}

void Game_Draw()
{
	Direct3D_SetOffBackBuffer();
	Direct3D_ClearBackBuffer();

	XMMATRIX view, proj;
	if (g_IsDebugCameraMode) {
		XMFLOAT4X4 v = DebugCamera_GetViewMatrix();
		XMFLOAT4X4 p = DebugCamera_GetProjectionMatrix();
		view = XMLoadFloat4x4(&v);
		proj = XMLoadFloat4x4(&p);
	}
	else {
		XMFLOAT4X4 v = Player_Camera_GetViewMatrix();
		XMFLOAT4X4 p = Player_Camera_GetProjectionMatrix();
		view = XMLoadFloat4x4(&v);
		proj = XMLoadFloat4x4(&p);
	}
	if (g_Player) {
		g_Player->Draw(view, proj);
	}
	Enemy_Draw(view, proj);

	// --- 绘制静态环境 (通用着色器) ---
	Shader_3D_Begin();
	Camera_SetMatrixToShader(view, proj);

	Light_SetAmbient({ 1.0f, 1.0f, 1.0f });
	Light_SetDirectionalWorld({ 0.0f, -1.0f, 0.0f, 0.0f }, { 0.3f, 0.3f, 0.3f, 1.0f });

	Sampler_SetFilterAnisotropic();
	Sky_Draw();
	Bullet_Draw();
	DropItem_Draw();
	Map_Draw(); 


	Direct3D_SetOffscreenTexture(0);
	Direct3D_SetDepthEnable(false);
	Sprite_Begin();
	GameUI_Draw();
	Inventory_Draw();
	UI_DrawHUD();
	Direct3D_SetDepthEnable(true);
}

void Game_Finalize()
{
	if (g_Player) {
		delete g_Player;
		g_Player = nullptr;
	}
	Inventory_Finalize();
	DropItem_Finalize();
	Enemy_Finalize();
	Sky_Finalize();
	Map_Finalize();
	Bullet_Finalize();
	Player_Camera_Finalize();
}

