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
#include"PlayerCharacter.h"
#include "enemy_test.h"
#include "Pathfinder.h"
#include "fade.h"
#include "scene.h"
#include "cube.h"
#include "score.h"
#include "NavigationSystem.h"
using namespace DirectX;

namespace
{
	bool g_IsDebugCameraMode = false;
	PlayerCharacter* g_Player = nullptr;
	const DirectX::XMFLOAT3 g_GoalPos = { 20.0f, 1.0f, 10.0f };
	MODEL* g_DoorModel = nullptr;
	double g_CurrentGameTime = 0.0;
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
	g_DoorModel = ModelLoad("resource/Model/Door.fbx", 0.05f);
	Map_Initialize(g_GoalPos);
	Bullet_Initialize();
	Sky_Initialize();
	Pathfinder::Initialize();
	NavigationSystem::Initialize();
	bool success = NavigationSystem::GetInstance()->Build();
	if (success) {
		OutputDebugStringA("=== NavMesh Build Success! ===\n");
	}
	else {
		OutputDebugStringA("=== NavMesh Build FAILED! ===\n");
	}
	Player_Camera_Initialize();
	Inventory_Initialize();
	DropItem_Initialize();
	g_CurrentGameTime = 0.0;

	if (!g_Player) { 
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

	float screenW = (float)Direct3D_GetBackBufferWidth();

	// 2. 设定参数
	int digits = 6;                // 显示6位数 (例如 000100)
	float fontSize = 32.0f;        // 分数数字的大小 (根据 score.cpp 里的定义)
	float margin = 20.0f;          // 距离边框的间距

	// 3. 计算右上角坐标
	// X = 屏幕宽 - (数字个数 * 单个数字宽) - 右边距
	float scoreX = screenW - (digits * fontSize) - margin;
	float scoreY = margin;         // 顶边距




	Score_Initialize(scoreX, scoreY, digits);

	// 5. 重置分数为0 (新游戏开始)
	Score_Reset();

	// 开始淡入，让画面亮起来
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
	Score_Update();
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

	if (g_Player && !g_Player->IsDead())
	{
		g_CurrentGameTime += elapsed_time;
	}

	if (g_Player && !g_Player->IsDead())
	{
		// 获取玩家和终点的包围盒
		AABB playerAABB = g_Player->GetAABB();
		AABB goalAABB;
		if (g_DoorModel) {
			goalAABB = ModelGetAABB(g_DoorModel, g_GoalPos);
		}
		else {
			goalAABB = Cube_CreateAABB(g_GoalPos);
		}

		if (Collision_IsOverLapAABB(playerAABB, goalAABB))
		{
			Score_SetTime(g_CurrentGameTime);
			Scene_Change(SCENE_RESULT);
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

	DirectX::XMMATRIX goalWorld = DirectX::XMMatrixTranslation(g_GoalPos.x, g_GoalPos.y, g_GoalPos.z);
	ModelDraw(g_DoorModel, goalWorld);


	Direct3D_SetOffscreenTexture(0);
	Direct3D_SetDepthEnable(false);
	Sprite_Begin();
	GameUI_Draw();
	Score_Draw();
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
	ModelRelease(g_DoorModel);
	g_DoorModel = nullptr;
	Inventory_Finalize();
	DropItem_Finalize();
	Enemy_Finalize();
	Sky_Finalize();
	NavigationSystem::Finalize();
	Map_Finalize();
	Bullet_Finalize();
	Player_Camera_Finalize();
}

