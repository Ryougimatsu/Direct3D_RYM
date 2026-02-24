// ======================================================================================
// Includes
// ======================================================================================
// System & Math
#include "game.h"
#include "direct3d.h"
#include "mouse.h"
#include "score.h"
#include "fade.h"
#include "scene.h"=
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <stdlib.h> 

// Graphics & Shaders
#include "shader.h"
#include "shader_3d.h"
#include "Shader_Shadow.h"
#include "SkinningShader.h"
#include "Sampler.h"
#include "Light.h"
#include "texture.h"
#include "billboard.h"
#include "camera.h"

// Game Systems
#include "map.h"
#include "Meshfield.h"
#include "sky.h"
#include "Pathfinder.h"
#include "NavigationSystem.h"
#include "Player_Camera.h"
#include "MapCamera.h"
#include "DebugCamera.h"
#include "GameUI.h"

// Game Objects & Models
#include "model.h"
#include "cube.h"
#include "SkinningModel.h"
#include "PlayerCharacter.h"
#include "enemy.h"
#include "enemy_test.h"
#include "bullet.h"
#include "bullet_hit_effect.h"
#include "DropItem.h"
#include "Inventory.h"
#include "sprite.h"
#include "sprite_anime.h"
#include "particle_system.h"
#include "Font.h"

using namespace DirectX;

// ======================================================================================
// Internal Globals & Constants
// ======================================================================================
namespace
{
	// 游戏状态
	bool g_IsDebugCameraMode = false;
	double g_CurrentGameTime = 0.0;
	bool g_IsPaused = false;
	// 关键对象
	PlayerCharacter* g_Player = nullptr;
	MODEL* g_DoorModel = nullptr;
	const DirectX::XMFLOAT3 g_GoalPos = { 20.0f, 1.0f, 10.0f };

	// 光源参数 (阴影生成用)
	// 位置设高一点以覆盖更广的区域
	XMVECTOR g_LightPos = XMVectorSet(10.0f, 25.0f, -5.0f, 1.0f);
	XMVECTOR g_LightTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR g_LightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	ParticleSystem* g_SmokeSystem = nullptr;
	int g_TexSmoke = -1;
	float g_SmokeTimer = 0.0f;

	int g_TexArrow = -1;
	int g_TexWhite = -1;
}

// ======================================================================================
// Helper Functions (Wrappers)
// ======================================================================================
extern float RandomFloat(float min, float max);
bool Game_IsLineOfSightBlocked(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end)
{
	return Map_CheckLineOfSightBlocked(start, end);
}

bool Game_CheckCollisionWithWalls(const AABB& objAabb)
{
	return Map_CheckCollision(objAabb);
}
// ======================================================================================
// Game Lifecycle: Load & Initialize
// ======================================================================================
void Game_LoadContent()
{
	// 1. 加载资源与静态模型
	g_DoorModel = ModelLoad("resource/Model/Door.fbx", 0.05f);

	// 2. 初始化子系统
	Map_Initialize(g_GoalPos);
	Sky_Initialize();
	Bullet_Initialize();
	Pathfinder::Initialize();
	NavigationSystem::Initialize();

	// 构建导航网格
	if (!NavigationSystem::GetInstance()->Build()) {
		OutputDebugStringA("[Game] Navigation Build Failed!\n");
	}

	// 3. 初始化摄像机与UI
	Player_Camera_Initialize();
	GameUI_Initialize();

	// 4. 初始化物品系统
	Inventory_Initialize();
	DropItem_Initialize();

	// 5. 初始化角色与敌人
	if (!g_Player) {
		g_Player = new PlayerCharacter();
		if (!g_Player->Initialize()) {
			OutputDebugStringA("[Game] PlayerCharacter Initialize Failed!\n");
		}
	}
	EnemyTest::LoadAssets();
	Enemy_Initialize();

	// 6. 重置时间
	g_CurrentGameTime = 0.0;

	g_TexSmoke = Texture_LoadFromFile(L"resource/texture/kenney_particle-pack/PNG (Transparent)/smoke_07.png");
	g_SmokeSystem = new ParticleSystem();
	g_SmokeSystem->Initialize(2000, g_TexSmoke);
	g_TexArrow = Texture_LoadFromFile(L"resource/texture/arrow.png");
	g_TexWhite = Texture_LoadFromFile(L"resource/texture/white.png");
}

void Game_Initialize()
{
	// 重置摄像机
	Camera_Initialize();
	DebugCamera_Initialize({ 0.0f, 5.0f, -10.0f }, { 0.0f, 0.0f, 0.0f });
	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
	// 重置玩家位置
	if (g_Player) {
		g_Player->SetPosition({ 0.0f, 0.0f, 0.0f });
	}

	// 初始化分数显示位置
	float screenW = (float)Direct3D_GetBackBufferWidth();
	int digits = 6;
	float fontSize = 32.0f;
	float margin = 20.0f;
	float scoreX = screenW - (digits * fontSize) - margin;
	float scoreY = margin;
	Score_Initialize(scoreX, scoreY, digits);
	Score_Reset();

	// 重置时间
	g_CurrentGameTime = 0.0;

	// 开始淡入
	Fade_Start(1.0, false, { 0.0f, 0.0f, 0.0f });
}

// ======================================================================================
// Game Lifecycle: Update
// ======================================================================================
void Game_Update(double elapsed_time)
{
	Inventory_Update(elapsed_time);
	if (Inventory_IsOpen()) {
		return;
	}

	if (KeyLogger_IsTrigger(KK_P)) {
		g_IsPaused = !g_IsPaused;
	}

	// --- 1. 输入处理与模式切换 ---
	if (KeyLogger_IsTrigger(KK_TAB))
	{
		g_IsDebugCameraMode = !g_IsDebugCameraMode;
		if (g_IsDebugCameraMode) {
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
			DebugCamera_SetPosition(Player_Camera_GetPosition());
		}
		else {
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		}
	}
	if (KeyLogger_IsTrigger(KK_LEFTALT))
	{
		Mouse_State mState;
		Mouse_GetState(&mState);

		if (mState.positionMode == MOUSE_POSITION_MODE_RELATIVE) {
			// 如果当前是隐藏(相对)模式，切换为显示(绝对)模式
			Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
		}
		else {
			// 如果当前是显示(绝对)模式，切换为隐藏(相对)模式
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		}
	}

	if (g_IsPaused) {
		if (g_IsDebugCameraMode) {
			DebugCamera_Update(elapsed_time);
		}
		return;
	}

	// --- 2. 摄像机更新 ---
	DirectX::XMFLOAT3 pPos = (g_Player) ? g_Player->GetPosition() : DirectX::XMFLOAT3{ 0,0,0 };

	if (g_IsDebugCameraMode) {
		DebugCamera_Update(elapsed_time);
	}
	else {
		Player_Camera_Update(elapsed_time, pPos);
	}

	// 摄像机跟随天空盒
	Sky_SetPosition(Player_Camera_GetPosition());

	// --- 3. 实体更新 ---
	Enemy_Update(elapsed_time);
	Bullet_Update(elapsed_time);
	Bullet_CheckCollisionWithEnemies();
	DropItem_Update(elapsed_time);
	Score_Update();
	g_SmokeSystem->Update(elapsed_time);
	g_SmokeTimer += (float)elapsed_time;

	// 玩家更新
	if (!g_IsDebugCameraMode && g_Player)
	{
		g_Player->Update(elapsed_time);

		// 死亡检测：动画播放完毕后切换场景
		if (g_Player->IsDeathAnimationFinished())
		{
			delete g_Player;
			g_Player = nullptr;
			Scene_Change(SCENE_GAMEOVER);
			return;
		}
	}

	// --- 4. 游戏逻辑判断 ---
	if (g_Player && !g_Player->IsDead())
	{
		g_CurrentGameTime += elapsed_time;

		// 胜利条件检测：到达目标点
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
	if (g_SmokeTimer > 0.2f) {
		g_SmokeTimer = 0.0f;

		// 在地图范围内随机选一个点
		// 假设地图大小是 -20 到 20
		float randX = RandomFloat(-20.0f, 20.0f);
		float randZ = RandomFloat(-20.0f, 20.0f);

		// 在地面 (Y=0) 生成 1 个烟雾粒子
		// 如果想看起来更浓，可以一次生成 3-5 个
		g_SmokeSystem->EmitSmoke({ randX, 0.0f, randZ }, 2);
	}
}

// ======================================================================================
// Game Lifecycle: Draw
// ======================================================================================
void Game_Draw()
{
	// ----------------------------------------------------------------
	// 0. 准备光源矩阵 (用于阴影和光照)
	// ----------------------------------------------------------------
	XMMATRIX lightView = XMMatrixLookAtLH(g_LightPos, g_LightTarget, g_LightUp);
	XMMATRIX lightProj = XMMatrixOrthographicLH(300.0f, 300.0f, 1.0f, 200.0f);
	XMMATRIX lightVP = lightView * lightProj;

	// ----------------------------------------------------------------
	// Pass 1: Shadow Map 生成 (只渲染深度)
	// ----------------------------------------------------------------
	Shader_Shadow_Begin(lightView, lightProj);
	{
		// 1. 绘制静态场景阴影 (墙壁)
		const std::vector<MapObject>& mapObjs = Map_GetObjects();
		for (const auto& obj : mapObjs) {
			if (obj.KindId == MAP_KIND_WALL) {
				XMMATRIX world = XMMatrixTranslation(obj.Position.x, obj.Position.y, obj.Position.z);
				Cube_DrawShadow(world);
			}
		}

		// 2. 绘制道具/门阴影
		if (g_DoorModel) {
			DirectX::XMMATRIX goalWorld = DirectX::XMMatrixTranslation(g_GoalPos.x, g_GoalPos.y, g_GoalPos.z);
			ModelDrawShadow(g_DoorModel, goalWorld);
		}
		DropItem_DrawShadow(lightView, lightProj);

		// 3. 绘制角色阴影 (Player & Enemy)
		if (g_Player) g_Player->DrawShadow(lightView, lightProj);
		Enemy_DrawShadow(lightView, lightProj);
	}
	Shader_Shadow_End();

	// ----------------------------------------------------------------
	// Pass 2: 正常场景渲染 (Main Pass)
	// ----------------------------------------------------------------
	Direct3D_SetOffBackBuffer();
	Direct3D_ClearBackBuffer();

	// 1. 获取摄像机矩阵
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

	// 设置全局相机参数
	Camera_SetMatrixToShader(view, proj);

	// 2. 绘制蒙皮动画角色 (Player & Enemy)
	// 注意：需要绑定 Shadow Map 到 Skinning Shader
	ID3D11ShaderResourceView* shadowSRV = Shader_Shadow_GetSRV();
	SkinningShader_3D_SetShadowResources(shadowSRV, lightVP);

	if (g_Player) g_Player->Draw(view, proj);
	Enemy_Draw(view, proj);

	// 3. 绘制静态 3D 物体 (Map, Items, Door)
	// 切换到通用 3D Shader
	Shader_3D_Begin();

	// 设置光照参数
	Light_SetAmbient({ 0.4f, 0.4f, 0.4f });
	XMVECTOR dirVec = XMVector3Normalize(g_LightTarget - g_LightPos);
	XMFLOAT4 lightDirF4;
	XMStoreFloat4(&lightDirF4, dirVec);
	Light_SetDirectionalWorld(lightDirF4, { 0.8f, 0.8f, 0.8f, 1.0f });

	// 设置阴影数据 (给 Shader_3D 使用)
	Shader_3D_SetLightData(lightVP, shadowSRV);

	// 执行绘制
	Map_Draw(lightVP, shadowSRV); // 内部绘制地面和墙壁
	DropItem_Draw();

	// 绘制门 (重新绑定数据以防被覆盖)
	Shader_3D_SetLightData(lightVP, shadowSRV);
	DirectX::XMMATRIX goalWorld = DirectX::XMMatrixTranslation(g_GoalPos.x, g_GoalPos.y, g_GoalPos.z);
	ModelDraw(g_DoorModel, goalWorld);

	// ----------------------------------------------------------------
	// Pass 3: 透明物体与天空 (Translucent & Sky)
	// ----------------------------------------------------------------
	// 解绑 Shadow Map SRV
	// 因为 Shadow Map 可能会在下一帧被当作 Render Target 写入，DX11 不允许同时作为 Input 和 Output
	ID3D11ShaderResourceView* nullSRV = nullptr;
	Direct3D_GetDeviceContext()->PSSetShaderResources(5, 1, &nullSRV);

	Sky_Draw();
	Bullet_Draw();
	Direct3D_SetBlendState(BLEND_MODE_ALPHA);
	Direct3D_SetDepthStencilStateDepthWriteDisable(false);
	g_SmokeSystem->Draw();
	Direct3D_SetDepthStencilStateDepthWriteDisable(true);
	// ----------------------------------------------------------------
	// Pass 4: UI & 2D 渲染
	// ----------------------------------------------------------------
	Direct3D_SetOffscreenTexture(0);
	Direct3D_SetDepthEnable(false);

	Sprite_Begin();
	{
		GameUI_Draw();
		Score_Draw();
		Inventory_Draw();
		UI_DrawHUD();
		if (g_Player && g_TexArrow != -1 && !g_IsDebugCameraMode)
		{
			// 1. 获取视口信息
			D3D11_VIEWPORT vp;
			UINT numVp = 1;
			Direct3D_GetDeviceContext()->RSGetViewports(&numVp, &vp);

			// 2. 获取当前的摄像机矩阵
			DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&Player_Camera_GetViewMatrix());
			DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&Player_Camera_GetProjectionMatrix());
			DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

			// 3. 获取玩家和目标点的 3D 坐标
			DirectX::XMFLOAT3 pPos = g_Player->GetPosition();
			pPos.y += 1.0f; // 抬高到角色胸口位置算投影更准
			DirectX::XMVECTOR vPlayerPos = DirectX::XMLoadFloat3(&pPos);

			// 取目标点坐标（强制高度与玩家一致，防止因为门太高导致屏幕方向算歪）
			DirectX::XMVECTOR vGoalPos = DirectX::XMLoadFloat3(&g_GoalPos);
			vGoalPos = DirectX::XMVectorSetY(vGoalPos, pPos.y);

			// 4. 将 3D 世界坐标投影到 2D 屏幕像素坐标！
			DirectX::XMVECTOR vScreenPlayer = DirectX::XMVector3Project(vPlayerPos, vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth, proj, view, world);

			// 为了防止门在摄像机背后时投影坐标翻转，我们只取玩家正前方朝向门的“1米处”来做屏幕方向计算
			DirectX::XMVECTOR vDirToGoal = DirectX::XMVector3Normalize(vGoalPos - vPlayerPos);
			DirectX::XMVECTOR vPointAhead = vPlayerPos + vDirToGoal * 1.0f;
			DirectX::XMVECTOR vScreenAhead = DirectX::XMVector3Project(vPointAhead, vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth, proj, view, world);

			// 5. 计算屏幕上的 2D 向量和旋转角度
			DirectX::XMVECTOR vScreenDir = vScreenAhead - vScreenPlayer;
			float screenDx = DirectX::XMVectorGetX(vScreenDir);
			float screenDy = DirectX::XMVectorGetY(vScreenDir);

			// atan2f 可以直接算出 2D 向量的弧度角
			float angle = atan2f(screenDy, screenDx);

			// 6. 让箭头环绕在玩家屏幕位置的周围 (半径 100 像素)
			float orbitRadius = 100.0f;
			float arrowCenterX = DirectX::XMVectorGetX(vScreenPlayer) + cosf(angle) * orbitRadius;
			float arrowCenterY = DirectX::XMVectorGetY(vScreenPlayer) + sinf(angle) * orbitRadius;

			// 7. 绘制旋转后的箭头 Sprite
			float arrowSize = 64.0f; // 箭头在屏幕上的大小

			//  arrow.png 贴图是(→)的。
			// 如果你的贴图默认是(↑)的，请把下面传入的 angle 改为 (angle + 1.57f)
			Sprite_Draw(g_TexArrow,
				arrowCenterX - arrowSize * 0.5f, arrowCenterY - arrowSize * 0.5f, // 左上角起点
				arrowSize, arrowSize, // 绘制宽高
				0, 0, 1024, 1024,       // 贴图UV裁剪
				angle,                // 旋转角度
				{ 1.0f, 0.8f, 0.2f, 0.8f } // 颜色和透明度 (这里用了一个半透明的橙黄色)
			);
		}
		if (g_IsPaused)
		{
			float screenW = (float)Direct3D_GetBackBufferWidth();
			float screenH = (float)Direct3D_GetBackBufferHeight();
			if (g_TexWhite != -1) {
				Sprite_Draw(g_TexWhite, 0.0f, 0.0f, screenW, screenH, { 0.0f, 0.0f, 0.0f, 0.5f });
			}

			float textX = screenW / 2.0f - 60.0f;
			float textY = screenH / 2.0f - 20.0f;
			Font_Draw(L"PAUSED", textX, textY, { 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}
	Sprite_End();

	Direct3D_SetDepthEnable(true);
}

// ======================================================================================
// Game Lifecycle: Finalize
// ======================================================================================
void Game_Finalize()
{
	// 释放角色
	if (g_Player) {
		delete g_Player;
		g_Player = nullptr;
	}
	if (g_SmokeSystem) {
		g_SmokeSystem->Finalize();
		delete g_SmokeSystem;
		g_SmokeSystem = nullptr;
	}
	// 释放静态模型
	ModelRelease(g_DoorModel);
	g_DoorModel = nullptr;

	// 释放子系统
	Inventory_Finalize();
	DropItem_Finalize();
	Enemy_Finalize();
	Sky_Finalize();
	NavigationSystem::Finalize();
	Map_Finalize();
	Bullet_Finalize();
	Player_Camera_Finalize();
}