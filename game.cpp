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
#include "PlayerCharacter.h"
#include "enemy_test.h"
#include "Pathfinder.h"
#include "fade.h"
#include "scene.h"
#include "cube.h"
#include "score.h"
#include "NavigationSystem.h"
#include "Shader_Shadow.h" // 引入阴影

using namespace DirectX;

namespace
{
	bool g_IsDebugCameraMode = false;
	PlayerCharacter* g_Player = nullptr;
	const DirectX::XMFLOAT3 g_GoalPos = { 20.0f, 1.0f, 10.0f };
	MODEL* g_DoorModel = nullptr;
	double g_CurrentGameTime = 0.0;

	// 光源参数 (定义在这里，Draw中直接使用)
	// 光源位置建议设高一点，以覆盖更大的阴影范围
	XMVECTOR g_LightPos = XMVectorSet(20.0f, 30.0f, -10.0f, 1.0f);
	// 简单的向下指
	XMVECTOR g_LightTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR g_LightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
}

bool Game_IsLineOfSightBlocked(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end)
{
	return Map_CheckLineOfSightBlocked(start, end);
}

bool Game_CheckCollisionWithWalls(const AABB& objAabb)
{
	return Map_CheckCollision(objAabb);
}

void Game_LoadContent()
{
	g_DoorModel = ModelLoad("resource/Model/Door.fbx", 0.05f);
	Map_Initialize(g_GoalPos);
	Bullet_Initialize();
	Sky_Initialize();
	Pathfinder::Initialize();
	NavigationSystem::Initialize();
	bool success = NavigationSystem::GetInstance()->Build();
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

	EnemyTest::LoadAssets();
	Enemy_Initialize();
	GameUI_Initialize();
}

void Game_Initialize()
{
	Camera_Initialize();
	DebugCamera_Initialize({ 0.0f, 5.0f, -10.0f }, { 0.0f, 0.0f, 0.0f });

	if (g_Player) {
		g_Player->SetPosition({ 0.0f, 0.0f, 0.0f });
	}

	float screenW = (float)Direct3D_GetBackBufferWidth();
	int digits = 6;
	float fontSize = 32.0f;
	float margin = 20.0f;
	float scoreX = screenW - (digits * fontSize) - margin;
	float scoreY = margin;

	Score_Initialize(scoreX, scoreY, digits);
	Score_Reset();
	Fade_Start(1.0, false, { 0.0f, 0.0f, 0.0f });
}

void Game_Update(double elapsed_time)
{
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

	DirectX::XMFLOAT3 pPos = { 0.0f, 0.0f, 0.0f };
	if (g_Player) {
		pPos = g_Player->GetPosition();
	}

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
	// =============================================================
	// 0. 准备光源矩阵
	// =============================================================
	XMMATRIX lightView = XMMatrixLookAtLH(g_LightPos, g_LightTarget, g_LightUp);
	XMMATRIX lightProj = XMMatrixOrthographicLH(160.0f, 160.0f, 1.0f, 200.0f);

	// =============================================================
	// Pass 1: Shadow Map 生成 (只渲染深度)
	// =============================================================
	Shader_Shadow_Begin(lightView, lightProj);

	// 1. 绘制墙壁
	const std::vector<MapObject>& mapObjs = Map_GetObjects();
	for (const auto& obj : mapObjs) {
		if (obj.KindId == MAP_KIND_WALL) {
			XMMATRIX world = XMMatrixTranslation(obj.Position.x, obj.Position.y, obj.Position.z);
			Cube_DrawShadow(world);
		}
	}

	// 2. 绘制门
	if (g_DoorModel) {
		DirectX::XMMATRIX goalWorld = DirectX::XMMatrixTranslation(g_GoalPos.x, g_GoalPos.y, g_GoalPos.z);
		ModelDrawShadow(g_DoorModel, goalWorld);
	}

	Shader_Shadow_End();

	// =============================================================
	// Pass 2: 正常场景渲染 (Main Pass)
	// =============================================================
	Direct3D_SetOffBackBuffer();
	Direct3D_ClearBackBuffer();

	// 获取相机矩阵
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

	// 1. 绘制角色 (不受阴影影响，先画)
	if (g_Player) g_Player->Draw(view, proj);
	Enemy_Draw(view, proj);

	// --- 进入 Shader_3D 渲染阶段 ---
	Shader_3D_Begin();

	// 准备通用参数
	XMMATRIX lightVP = lightView * lightProj;
	Camera_SetMatrixToShader(view, proj);
	Light_SetAmbient({ 0.4f, 0.4f, 0.4f });
	XMVECTOR dirVec = XMVector3Normalize(g_LightTarget - g_LightPos);
	XMFLOAT4 lightDirF4;
	XMStoreFloat4(&lightDirF4, dirVec);
	Light_SetDirectionalWorld(lightDirF4, { 0.8f, 0.8f, 0.8f, 1.0f });

	// ---------------------------------------------------------
	// 第一阶段：绑定阴影图，绘制地图
	// ---------------------------------------------------------
	Shader_3D_SetLightData(lightVP, Shader_Shadow_GetSRV());
	Map_Draw();

	// ---------------------------------------------------------
	// 第二阶段：绘制掉落物 (它可能会修改 Slot 1 导致阴影图失效!)
	// ---------------------------------------------------------
	DropItem_Draw();

	// ---------------------------------------------------------
	// 【关键修复】第三阶段：必须重新绑定阴影图！
	// 因为上面的 DropItem_Draw 刚刚把 Slot 1 弄脏了
	// ---------------------------------------------------------
	Shader_3D_SetLightData(lightVP, Shader_Shadow_GetSRV());

	// ---------------------------------------------------------
	// 第四阶段：绘制门 (现在它能读到正确的阴影图了，不会报错)
	// ---------------------------------------------------------
	DirectX::XMMATRIX goalWorld = DirectX::XMMatrixTranslation(g_GoalPos.x, g_GoalPos.y, g_GoalPos.z);
	ModelDraw(g_DoorModel, goalWorld);


	// ---------------------------------------------------------
	// 第五阶段：绘制天空和子弹 (最后画)
	// ---------------------------------------------------------

	// 解绑 Shadow Map SRV (必须解绑，否则下一帧 Pass 1 无法写入)
	ID3D11ShaderResourceView* nullSRV = nullptr;
	Direct3D_GetDeviceContext()->PSSetShaderResources(1, 1, &nullSRV);

	Sky_Draw();
	Bullet_Draw();

	// =============================================================
	// UI & 2D 渲染
	// =============================================================
	Direct3D_SetOffscreenTexture(0);
	Direct3D_SetDepthEnable(false);

	Sprite_Begin();
	GameUI_Draw();
	Score_Draw();
	Inventory_Draw();
	UI_DrawHUD();
	Sprite_End();

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