// ======================================================================================
// 头文件引用
// ======================================================================================
// 系统与数学库
#include "game.h"
#include "direct3d.h"
#include "mouse.h"
#include "score.h"
#include "fade.h"
#include "scene.h"
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <stdlib.h>

// 图形与着色器
#include "shader.h"
#include "shader_3d.h"
#include "Shader_Shadow.h"
#include "SkinningShader.h"
#include "Sampler.h"
#include "Light.h"
#include "texture.h"
#include "billboard.h"
#include "camera.h"

// 游戏子系统
#include "map.h"
#include "Meshfield.h"
#include "sky.h"
#include "Pathfinder.h"
#include "NavigationSystem.h"
#include "Player_Camera.h"
#include "MapCamera.h"
#include "DebugCamera.h"
#include "GameUI.h"

// 游戏对象与模型
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
// 匿名命名空间：模块内部全局变量与常量
// ======================================================================================
namespace
{
	// --- 游戏运行状态 ---
	bool   g_IsDebugCameraMode = false; // 是否处于调试摄像机模式（TAB 切换）
	double g_CurrentGameTime   = 0.0;   // 当前局游戏累计时间（秒），用于计分
	bool   g_IsPaused = false;          // 暂停标志（P 键切换）

	// --- 核心游戏对象 ---
	PlayerCharacter*  g_Player    = nullptr;                      // 玩家角色实例
	MODEL*            g_DoorModel = nullptr;                      // 关卡出口门的静态模型
	DirectX::XMFLOAT3 g_GoalPos   = { 20.0f, 1.0f, 10.0f };     // 出口门的世界坐标（每局随机生成）

	// --- 光源参数（用于阴影贴图生成）---
	XMVECTOR g_LightPos    = XMVectorSet(10.0f, 25.0f, -5.0f, 1.0f); // 平行光源位置（高位以覆盖更广阴影范围）
	XMVECTOR g_LightTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);    // 光源注视目标（场景中心）
	XMVECTOR g_LightUp     = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);    // 光源视角的上方向

	// --- 战场硝烟粒子系统 ---
	ParticleSystem* g_SmokeSystem = nullptr; // 环境烟雾粒子系统（营造战场氛围）
	int   g_TexSmoke  = -1;                  // 烟雾粒子纹理 ID
	float g_SmokeTimer = 0.0f;               // 烟雾发射间隔计时器

	// --- UI 纹理资源 ---
	int g_TexArrow = -1; // 目标指引箭头纹理（60 秒后显示）
	int g_TexWhite = -1; // 白色纯色纹理（暂停遮罩用）
}

// ======================================================================================
// 辅助函数（将地图模块的碰撞/视线功能包装为游戏层接口）
// ======================================================================================
extern float RandomFloat(float min, float max); // 定义在其他编译单元

// 检测两点之间的视线是否被墙壁阻挡（供敌人 AI 判断能否看到玩家）
bool Game_IsLineOfSightBlocked(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end)
{
	return Map_CheckLineOfSightBlocked(start, end);
}

// 检测指定 AABB 是否与地图墙壁发生碰撞（供角色/敌人移动时的碰撞检测）
bool Game_CheckCollisionWithWalls(const AABB& objAabb)
{
	return Map_CheckCollision(objAabb);
}
// ======================================================================================
// 游戏生命周期：资源加载（仅在场景首次进入时调用一次）
// ======================================================================================
void Game_LoadContent()
{
	// 1. 加载静态 3D 模型
	g_DoorModel = ModelLoad("resource/Model/Door.fbx", 0.05f); // 出口门模型

	// 2. 初始化核心子系统
	Sky_Initialize();                // 天空盒
	Bullet_Initialize();             // 子弹池
	Pathfinder::Initialize();        // A* 网格寻路
	NavigationSystem::Initialize();  // Recast/Detour 导航网格

	// 3. 初始化摄像机与 UI
	Player_Camera_Initialize();      // 玩家跟随摄像机
	GameUI_Initialize();             // 游戏界面元素（血条、弹药等）

	// 4. 初始化物品系统
	Inventory_Initialize();          // 背包/道具管理
	DropItem_Initialize();           // 地面掉落物

	// 5. 创建玩家与加载敌人资源
	if (!g_Player) {
		g_Player = new PlayerCharacter();
		if (!g_Player->Initialize()) {
			OutputDebugStringA("[Game] PlayerCharacter Initialize Failed!\n");
		}
	}
	EnemyTest::LoadAssets();  // 预加载敌人共享模型与动画（在 main.cpp 中统一 UnloadAssets）
	Enemy_Initialize();       // 初始化敌人管理器

	// 6. 重置游戏计时
	g_CurrentGameTime = 0.0;

	// 7. 加载粒子与 UI 纹理资源
	g_TexSmoke = Texture_LoadFromFile(L"resource/texture/kenney_particle-pack/PNG (Transparent)/smoke_07.png");
	g_SmokeSystem = new ParticleSystem();
	g_SmokeSystem->Initialize(2000, g_TexSmoke); // 2000 个烟雾粒子池
	g_TexArrow = Texture_LoadFromFile(L"resource/texture/arrow.png");  // 目标指引箭头
	g_TexWhite = Texture_LoadFromFile(L"resource/texture/white.png");  // 暂停遮罩用纯色纹理
}

// ======================================================================================
// 游戏生命周期：每局初始化（可重复调用，每次新局重置状态）
// ======================================================================================
void Game_Initialize()
{
	// 重置摄像机到默认视角
	Camera_Initialize();
	DebugCamera_Initialize({ 0.0f, 5.0f, -10.0f }, { 0.0f, 0.0f, 0.0f });
	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE); // 隐藏鼠标，使用相对模式

	// 重置玩家到出生点
	if (g_Player) {
		g_Player->SetPosition({ 0.0f, 0.0f, 0.0f });
	}

	// 随机生成出口门的位置（极坐标方式，保证距出生点 35~50 米）
	float minDistance = 35.0f;
	float maxDistance = 50.0f;
	float angle = RandomFloat(0.0f, 3.1415926f * 2.0f);
	float dist  = RandomFloat(minDistance, maxDistance);

	g_GoalPos.x = cosf(angle) * dist;
	g_GoalPos.z = sinf(angle) * dist;
	g_GoalPos.y = 1.0f; // 门的固定高度

	// 根据出口门位置初始化地图（墙壁布局等）
	Map_Initialize(g_GoalPos);

	// 构建 Recast/Detour 导航网格（需在地图初始化之后）
	if (!NavigationSystem::GetInstance()->Build()) {
		OutputDebugStringA("[Game] Navigation Build Failed!\n");
	}

	// 初始化计分板：右上角显示，6 位数字
	float screenW = (float)Direct3D_GetBackBufferWidth();
	int digits = 6;
	float fontSize = 32.0f;
	float margin = 20.0f;
	float scoreX = screenW - (digits * fontSize) - margin;
	float scoreY = margin;
	Score_Initialize(scoreX, scoreY, digits);
	Score_Reset();

	// 重置本局计时
	g_CurrentGameTime = 0.0;

	// 场景切入时播放淡入效果（黑色 → 透明，1 秒）
	Fade_Start(1.0, false, { 0.0f, 0.0f, 0.0f });
}

// ======================================================================================
// 游戏生命周期：每帧更新
// ======================================================================================
void Game_Update(double elapsed_time)
{
	// 背包系统优先更新（打开背包时冻结其他逻辑）
	Inventory_Update(elapsed_time);
	if (Inventory_IsOpen()) {
		return; // 背包界面打开时暂停游戏逻辑
	}

	// P 键切换暂停
	if (KeyLogger_IsTrigger(KK_P)) {
		g_IsPaused = !g_IsPaused;
	}

	// --- 1. 输入处理与调试模式切换 ---
	// TAB 键：在玩家摄像机与调试自由摄像机之间切换
	if (KeyLogger_IsTrigger(KK_TAB))
	{
		g_IsDebugCameraMode = !g_IsDebugCameraMode;
		if (g_IsDebugCameraMode) {
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
			DebugCamera_SetPosition(Player_Camera_GetPosition()); // 从玩家摄像机位置接管
		}
		else {
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		}
	}
	// ALT 键：切换鼠标显示/隐藏（用于窗口外操作）
	if (KeyLogger_IsTrigger(KK_LEFTALT))
	{
		Mouse_State mState;
		Mouse_GetState(&mState);

		if (mState.positionMode == MOUSE_POSITION_MODE_RELATIVE) {
			Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE); // 显示鼠标
		}
		else {
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE); // 隐藏鼠标
		}
	}

	// 暂停时仅更新调试摄像机，跳过所有游戏逻辑
	if (g_IsPaused) {
		if (g_IsDebugCameraMode) {
			DebugCamera_Update(elapsed_time);
		}
		return;
	}

	// --- 2. 摄像机更新 ---
	DirectX::XMFLOAT3 pPos = (g_Player) ? g_Player->GetPosition() : DirectX::XMFLOAT3{ 0,0,0 };

	if (g_IsDebugCameraMode) {
		DebugCamera_Update(elapsed_time);     // 自由飞行调试摄像机
	}
	else {
		Player_Camera_Update(elapsed_time, pPos); // 跟随玩家的第三人称摄像机
	}

	Sky_SetPosition(Player_Camera_GetPosition()); // 天空盒始终跟随摄像机（无限远效果）

	// --- 3. 游戏实体逐一更新 ---
	Enemy_Update(elapsed_time);             // 敌人 AI 状态机 + 移动 + 攻击
	Bullet_Update(elapsed_time);            // 子弹飞行 + 生命周期
	Bullet_CheckCollisionWithEnemies();     // 子弹与敌人的碰撞检测
	DropItem_Update(elapsed_time);          // 掉落物旋转/拾取检测
	Score_Update();                         // 分数滚动动画
	g_SmokeSystem->Update(elapsed_time);    // 环境烟雾粒子更新
	g_SmokeTimer += (float)elapsed_time;

	// 玩家更新（调试摄像机模式下跳过玩家输入）
	if (!g_IsDebugCameraMode && g_Player)
	{
		g_Player->Update(elapsed_time);

		// 死亡检测：死亡动画 + 倒计时结束后切换到 GameOver 场景
		if (g_Player->IsDeathAnimationFinished())
		{
			//delete g_Player;
			//g_Player = nullptr;
			Scene_Change(SCENE_GAMEOVER);
			return;
		}
	}

	// --- 4. 游戏胜利条件判断 ---
	if (g_Player && !g_Player->IsDead())
	{
		g_CurrentGameTime += elapsed_time; // 存活时累计游戏时间

		// 检测玩家是否到达出口门（AABB 重叠判定）
		AABB playerAABB = g_Player->GetAABB();
		AABB goalAABB;
		if (g_DoorModel) {
			goalAABB = ModelGetAABB(g_DoorModel, g_GoalPos); // 使用门模型的实际 AABB
		}
		else {
			goalAABB = Cube_CreateAABB(g_GoalPos); // 备用：简单立方体碰撞盒
		}

		if (Collision_IsOverLapAABB(playerAABB, goalAABB))
		{
			Score_SetTime(g_CurrentGameTime); // 记录通关时间
			Scene_Change(SCENE_RESULT);       // 切换到结算场景
		}
	}

	// --- 5. 环境烟雾粒子定期发射（每 0.2 秒一波）---
	if (g_SmokeTimer > 0.2f) {
		g_SmokeTimer = 0.0f;

		// 在玩家周围可见范围内随机生成烟雾（半径约 40 米）
		DirectX::XMFLOAT3 pPos = { 0.0f, 0.0f, 0.0f };
		if (g_Player) {
			pPos = g_Player->GetPosition();
		}

		float randX = pPos.x + RandomFloat(-40.0f, 40.0f);
		float randZ = pPos.z + RandomFloat(-30.0f, 30.0f);

		g_SmokeSystem->EmitSmoke({ randX, 0.0f, randZ }, 3); // 每波发射 3 个烟雾粒子
	}
}

// ======================================================================================
// 游戏生命周期：每帧绘制（分为 4 个渲染 Pass）
// ======================================================================================
void Game_Draw()
{
	// ----------------------------------------------------------------
	// Pass 0：计算光源视角矩阵（后续阴影贴图和光照计算共用）
	// ----------------------------------------------------------------
	XMMATRIX lightView = XMMatrixLookAtLH(g_LightPos, g_LightTarget, g_LightUp);
	XMMATRIX lightProj = XMMatrixOrthographicLH(450.0f, 450.0f, 1.0f, 200.0f); // 正交投影（覆盖整个地图）
	XMMATRIX lightVP   = lightView * lightProj;

	// ----------------------------------------------------------------
	// Pass 1：阴影贴图生成（从光源视角渲染场景深度到 Shadow Map RT）
	// ----------------------------------------------------------------
	Shader_Shadow_Begin(lightView, lightProj);
	{
		DirectX::XMFLOAT3 pPos = (g_Player) ? g_Player->GetPosition() : DirectX::XMFLOAT3{ 0,0,0 };

		// 1-1. 绘制静态墙壁的阴影（带距离剔除，60 米外的墙壁不投影以节省开销）
		const std::vector<MapObject>& mapObjs = Map_GetObjects();
		for (const auto& obj : mapObjs) {
			if (obj.KindId == MAP_KIND_WALL) {
				if (fabsf(obj.Position.x - pPos.x) > 60.0f ||
					fabsf(obj.Position.z - pPos.z) > 60.0f)
				{
					continue; // 超出剔除范围，跳过
				}

				XMMATRIX world = XMMatrixTranslation(obj.Position.x, obj.Position.y, obj.Position.z);
				Cube_DrawShadow(world);
			}
		}

		// 1-2. 绘制门和掉落物的阴影
		if (g_DoorModel) {
			DirectX::XMMATRIX goalWorld = DirectX::XMMatrixTranslation(g_GoalPos.x, g_GoalPos.y, g_GoalPos.z);
			ModelDrawShadow(g_DoorModel, goalWorld);
		}
		DropItem_DrawShadow(lightView, lightProj);

		// 1-3. 绘制动态角色阴影（玩家 + 所有敌人）
		if (g_Player) g_Player->DrawShadow(lightView, lightProj);
		Enemy_DrawShadow(lightView, lightProj);
	}
	Shader_Shadow_End();

	// ----------------------------------------------------------------
	// Pass 2：主场景渲染（从玩家摄像机视角绘制所有不透明物体）
	// ----------------------------------------------------------------
	Direct3D_SetOffBackBuffer();   // 切换回主交换链的后台缓冲
	Direct3D_ClearBackBuffer();    // 清除颜色与深度缓冲

	// 2-1. 根据当前模式获取对应摄像机的 View/Projection 矩阵
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

	Camera_SetMatrixToShader(view, proj); // 将摄像机矩阵推送到所有着色器

	// 取得 Pass 1 生成的阴影贴图 SRV，供后续着色器采样
	ID3D11ShaderResourceView* shadowSRV = Shader_Shadow_GetSRV();

	// 2-2. 启用 3D 静态物体着色器，绘制地图、掉落物、门
	Shader_3D_Begin();

	// 设置全局光照参数：环境光 + 平行光方向与颜色
	Light_SetAmbient({ 0.4f, 0.4f, 0.4f });
	XMVECTOR dirVec = XMVector3Normalize(g_LightTarget - g_LightPos);
	XMFLOAT4 lightDirF4;
	XMStoreFloat4(&lightDirF4, dirVec);
	Light_SetDirectionalWorld(lightDirF4, { 0.8f, 0.8f, 0.8f, 1.0f });

	// 绘制地图墙壁与掉落物（带阴影采样）
	Shader_3D_SetLightData(lightVP, shadowSRV);
	Map_Draw(lightVP, shadowSRV);
	DropItem_Draw();

	// 绘制出口门模型
	Shader_3D_SetLightData(lightVP, shadowSRV);
	DirectX::XMMATRIX goalWorld = DirectX::XMMatrixTranslation(g_GoalPos.x, g_GoalPos.y, g_GoalPos.z);
	ModelDraw(g_DoorModel, goalWorld);

	// 2-3. 绘制骨骼动画角色（玩家 + 敌人，使用 Skinning Shader + 阴影）
	SkinningShader_3D_SetShadowResources(shadowSRV, lightVP);
	if (g_Player) g_Player->Draw(view, proj);
	Enemy_Draw(view, proj);

	// ----------------------------------------------------------------
	// Pass 3：透明物体与天空盒（需在不透明物体之后绘制）
	// ----------------------------------------------------------------
	// 解绑 Shadow Map SRV，防止 DX11 资源冲突（同一资源不可同时作为 Input 和 Output）
	ID3D11ShaderResourceView* nullSRV = nullptr;
	Direct3D_GetDeviceContext()->PSSetShaderResources(5, 1, &nullSRV);

	Sky_Draw();    // 天空盒（最远层，利用深度缓冲自然被遮挡）
	Bullet_Draw(); // 子弹模型

	// 烟雾粒子：使用 Alpha 混合 + 关闭深度写入（半透明粒子不应遮挡后续透明物体）
	Direct3D_SetBlendState(BLEND_MODE_ALPHA);
	Direct3D_SetDepthStencilStateDepthWriteDisable(false);
	g_SmokeSystem->Draw();
	Direct3D_SetDepthStencilStateDepthWriteDisable(true); // 恢复深度写入

	// ----------------------------------------------------------------
	// Pass 4：2D UI 层（关闭深度测试，直接覆盖在 3D 场景之上）
	// ----------------------------------------------------------------
	Direct3D_SetDepthEnable(false);  // 2D UI 不参与深度测试

	Sprite_Begin();
	{
		GameUI_Draw();           // 游戏 UI（血条、弹药显示等）
		Score_Draw();            // 右上角分数
		Inventory_Draw();        // 背包界面（如果打开的话）
		UI_DrawHUD();            // HUD 抬头显示
		Player_DrawDamageFlash(); // 受伤红屏闪光

		// ---- 目标指引箭头（游戏进行 60 秒后显示，引导玩家找到出口门）----
		if (g_Player && g_TexArrow != -1 && !g_IsDebugCameraMode && g_CurrentGameTime >= 60.0)
		{
			// 获取视口参数
			D3D11_VIEWPORT vp;
			UINT numVp = 1;
			Direct3D_GetDeviceContext()->RSGetViewports(&numVp, &vp);

			// 获取当前摄像机矩阵
			DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&Player_Camera_GetViewMatrix());
			DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&Player_Camera_GetProjectionMatrix());
			DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

			// 获取玩家和目标的 3D 坐标
			DirectX::XMFLOAT3 pPos = g_Player->GetPosition();
			pPos.y += 1.0f; // 抬高到胸口位置，使投影更准确
			DirectX::XMVECTOR vPlayerPos = DirectX::XMLoadFloat3(&pPos);

			// 目标点强制与玩家同高（防止门模型高度差导致屏幕方向计算偏差）
			DirectX::XMVECTOR vGoalPos = DirectX::XMLoadFloat3(&g_GoalPos);
			vGoalPos = DirectX::XMVectorSetY(vGoalPos, pPos.y);

			// 将 3D 世界坐标投影到 2D 屏幕像素坐标
			DirectX::XMVECTOR vScreenPlayer = DirectX::XMVector3Project(vPlayerPos, vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth, proj, view, world);

			// 用玩家前方 1 米处的投影点计算屏幕方向（避免目标在摄像机背后时投影翻转）
			DirectX::XMVECTOR vDirToGoal  = DirectX::XMVector3Normalize(vGoalPos - vPlayerPos);
			DirectX::XMVECTOR vPointAhead = vPlayerPos + vDirToGoal * 1.0f;
			DirectX::XMVECTOR vScreenAhead = DirectX::XMVector3Project(vPointAhead, vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth, proj, view, world);

			// 计算屏幕上的 2D 方向向量和旋转角度
			DirectX::XMVECTOR vScreenDir = vScreenAhead - vScreenPlayer;
			float screenDx = DirectX::XMVectorGetX(vScreenDir);
			float screenDy = DirectX::XMVectorGetY(vScreenDir);
			float angle = atan2f(screenDy, screenDx); // 屏幕方向角（弧度）

			// 箭头环绕在玩家屏幕位置周围（半径 100 像素的圆轨道）
			float orbitRadius  = 100.0f;
			float arrowCenterX = DirectX::XMVectorGetX(vScreenPlayer) + cosf(angle) * orbitRadius;
			float arrowCenterY = DirectX::XMVectorGetY(vScreenPlayer) + sinf(angle) * orbitRadius;

			// 绘制箭头 Sprite（arrow.png 贴图默认朝右 →，angle 直接作为旋转角）
			float arrowSize = 64.0f;
			Sprite_Draw(g_TexArrow,
				arrowCenterX - arrowSize * 0.5f, arrowCenterY - arrowSize * 0.5f,
				arrowSize, arrowSize,
				0, 0, 1024, 1024,
				angle,
				{ 1.0f, 0.8f, 0.2f, 0.8f } // 半透明橙黄色
			);
		}

		// ---- 暂停画面遮罩 ----
		if (g_IsPaused)
		{
			float screenW = (float)Direct3D_GetBackBufferWidth();
			float screenH = (float)Direct3D_GetBackBufferHeight();
			if (g_TexWhite != -1) {
				Sprite_Draw(g_TexWhite, 0.0f, 0.0f, screenW, screenH, { 0.0f, 0.0f, 0.0f, 0.5f }); // 半透明黑色覆盖
			}

			float textX = screenW / 2.0f - 60.0f;
			float textY = screenH / 2.0f - 20.0f;
			Font_Draw(L"PAUSED", textX, textY, {1.0f, 1.0f, 1.0f, 1.0f}); // 居中白色文字
		}
	}
	Sprite_End();

	Direct3D_SetDepthEnable(true); // 恢复深度测试（为下一帧准备）
}

// ======================================================================================
// 游戏生命周期：场景卸载与资源释放
// ======================================================================================
void Game_Finalize()
{
	// 释放玩家角色实例（析构函数内会自动清理粒子系统 + 清空 g_pPlayerInstance）
	if (g_Player) {
		delete g_Player;
		g_Player = nullptr;
	}

	// 释放环境烟雾粒子系统
	if (g_SmokeSystem) {
		g_SmokeSystem->Finalize();
		delete g_SmokeSystem;
		g_SmokeSystem = nullptr;
	}

	// 释放门的静态模型
	ModelRelease(g_DoorModel);
	g_DoorModel = nullptr;

	// 逆序释放各子系统（与初始化顺序相反，避免依赖问题）
	Inventory_Finalize();           // 背包系统
	DropItem_Finalize();            // 掉落物
	Enemy_Finalize();               // 敌人管理器
	Sky_Finalize();                 // 天空盒
	NavigationSystem::Finalize();   // Recast/Detour 导航网格
	Map_Finalize();                 // 地图与墙壁
	Bullet_Finalize();              // 子弹池
	Player_Camera_Finalize();       // 玩家摄像机

	// 注意：纹理 ID（g_TexSmoke / g_TexArrow / g_TexWhite）由全局纹理管理器统一释放，
	// 不在此处单独释放。共享模型资源（PlayerCharacter::UnloadAssets / EnemyTest::UnloadAssets）
	// 在 main.cpp 退出时统一调用。
}
