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
using namespace DirectX;

namespace
{
	bool g_IsDebugCameraMode = false;
	PlayerCharacter* g_Player = nullptr;

}

// ------------------------------------------------------------------
// 初始化：仅保留核心环境和你的测试主角
// ------------------------------------------------------------------
void Game_Initialize()
{
	Camera_Initialize();
	DebugCamera_Initialize({ 0.0f, 5.0f, -10.0f }, { 0.0f, 0.0f, 0.0f });
	Bullet_Initialize();
	Sky_Initialize();
	Map_Initialize();         
	Player_Camera_Initialize();

	// 创建并初始化你的状态机角色
	g_Player = new PlayerCharacter();
	if (!g_Player->Initialize()) {  
		OutputDebugStringA("[Game] PlayerCharacter Initialize Failed!\n");
	}
	EnemyTest::LoadAssets();
	Enemy_Initialize();
	GameUI_Initialize();

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

	
	if (!g_IsDebugCameraMode && g_Player)
	{
		g_Player->Update(elapsed_time);
	}
}

// ------------------------------------------------------------------
// 绘制：只渲染环境和你的主角
// ------------------------------------------------------------------
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

	// --- 绘制你的主角 (蒙皮着色器) ---
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
	Map_Draw(); // 此时 Map 只会画地面

	Direct3D_SetOffscreenTexture(0);
	Direct3D_SetDepthEnable(false);
	Sprite_Begin();
	GameUI_Draw();
	Direct3D_SetDepthEnable(true);
}

void Game_Finalize()
{
	if (g_Player) {
		delete g_Player;
		g_Player = nullptr;
	}
	Enemy_Finalize();
	Sky_Finalize();
	Map_Finalize();
	Bullet_Finalize();
	Player_Camera_Finalize();
}