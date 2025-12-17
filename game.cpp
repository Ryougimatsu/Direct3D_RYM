#include "game.h"
#include "shader.h"
#include "Sampler.h"
#include "Meshfield.h"
#include "Light.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "model.h"
#include "camera.h"
#include "Player.h"
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


namespace 
{
	bool g_IsDebugCameraMode = false;
}

void Game_RenderMiniMap()
{
	Direct3D_SetOffscreen();
	XMFLOAT4X4 mtxView = MapCam_GetViewMatrix();
	XMFLOAT4X4 mtxProj = MapCam_GetPerspectiveMatrix();

	XMMATRIX view = XMLoadFloat4x4(&mtxView);
	XMMATRIX proj = XMLoadFloat4x4(&mtxProj);
	Camera_SetMatrixToShader(view, proj);


	Direct3D_SetDepthEnable(true); 
	Light_SetAmbient({ 1.0f, 1.0f, 1.0f }); 
	Map_Draw();
	Enemy_Draw();
	Player_Draw();
}
void Game_Initialize()
{
	Camera_Initialize();
	DebugCamera_Initialize({ 0.0f, 5.0f, -5.0f }, { 0.0f, 0.0f, 0.0f });
	Sky_Initialize();
	Bullet_Initialize();
	BulletHitEffect_Initialize();
	Player_Initialize({ 0.0f, 3.0f, 0.0f }, { 0.0f,0.0f,1.0f });
	Enemy_Initialize();
	Player_Camera_Initialize();
	Map_Initialize();
	Billboard_Initialize();
	Enemy_Create({-3.0f,1.0f,5.0f});
	Inventory_Initialize();
}

void Game_Update(double elapsed_time)
{

	//Player_Update(elapsed_time);
	XMFLOAT3 playerPos = Player_GetPosition();
	Enemy_Update(elapsed_time);
	//Player_Camera_Update(elapsed_time);
	SpriteAnime_Update(elapsed_time);
	MapCam_SetPosition({ playerPos.x, 25.0f, playerPos.z });
	MapCam_SetFront({ 0.0f, -1.0f, 0.0f });
	Sky_SetPosition(Player_Camera_GetPosition());
	Bullet_Update(elapsed_time);
	BulletHitEffect_Update();
	Inventory_Update(elapsed_time);
	if (KeyLogger_IsTrigger(KK_TAB))
	{
		g_IsDebugCameraMode = !g_IsDebugCameraMode;

		if (g_IsDebugCameraMode)
		{
			// 1. 开启相对模式 (隐藏鼠标，锁定在中心，提供 Delta 值)
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);

			// 2. (可选) 切换时把自由相机传送到玩家当前位置，实现无缝切换
			// DebugCamera_SetPosition(Player_Camera_GetPosition());
		}
		else
		{
			// 1. 恢复绝对模式 (显示鼠标，正常操作 UI)
			Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
		}
	}

		if (g_IsDebugCameraMode)
		{
			DebugCamera_Update(elapsed_time);
		}
		else
		{
			Player_Update(elapsed_time);
			Player_Camera_Update(elapsed_time);
		}

		for (int j = 0; j < Map_GetObjectsCount(); j++)
		{
			// 获取地图物体指针，以便判断类型
			const MapObject* pMapObj = Map_GetObject(j);

			for (int i = 0; i < Bullet_GetCount(); i++)
			{
				AABB bulletAABB = Bullet_GetAABB(i);
				AABB mapObjAABB = pMapObj->Aabb;

				if (Collision_IsOverLapAABB(bulletAABB, mapObjAABB))
				{
					// [新增] 在销毁子弹前，生成命中特效
					// 使用子弹当前的位置 (center) 作为特效生成点
					// 这样无论是打中树木(KindId=2)还是箱子(KindId=1)，都会有火花
					BulletHitEffect_Create(Bullet_GetSphere(i).center);

					// 如果你想让地面(KindId=0)不产生特效，可以加个判断：
					// if (pMapObj->KindId != 0) BulletHitEffect_Create(...);

					Bullet_Destroy(i);
					break;
				}
			}
		}
		for (int j = 0; j < Enemy_GetEnemyCount(); j++)
		{
			for (int i = 0; i < Bullet_GetCount(); i++)
			{
				Sphere bullet = Bullet_GetSphere(i);
				Sphere enemy = Enemy_GetEnemy(j)->GetCollisionSphere();
				if (Collision_IsOverlapSphere(bullet, enemy))
				{
					BulletHitEffect_Create(Bullet_GetSphere(i).center);
					Enemy_GetEnemy(j)->Damage(50.0f);
					Bullet_Destroy(i);
				}
			}
		}

}
void Game_Draw()
{
	Game_RenderMiniMap();
	Direct3D_SetOffBackBuffer();    // 切回主屏幕
	Direct3D_ClearBackBuffer();


	XMMATRIX view, proj;
	XMFLOAT3 camPos;
	if (g_IsDebugCameraMode)
	{
		// 自由视角
		XMFLOAT4X4 v = DebugCamera_GetViewMatrix();
		XMFLOAT4X4 p = DebugCamera_GetProjectionMatrix();
		view = XMLoadFloat4x4(&v);
		proj = XMLoadFloat4x4(&p);
		camPos = DebugCamera_GetPosition();
	}
	else
	{
		// 玩家视角
		XMFLOAT4X4 v = Player_Camera_GetViewMatrix();
		XMFLOAT4X4 p = Player_Camera_GetProjectionMatrix();
		view = XMLoadFloat4x4(&v);
		proj = XMLoadFloat4x4(&p);
		camPos = Player_Camera_GetPosition();
	}
	//XMFLOAT4X4 mtxView = Player_Camera_GetViewMatrix();
	//XMMATRIX view = XMLoadFloat4x4(&mtxView);
	//XMMATRIX proj = XMLoadFloat4x4(&Player_Camera_GetProjectionMatrix());
	Camera_SetMatrixToShader(view, proj);
	XMMATRIX mtxWorld = XMMatrixIdentity();

	XMFLOAT3 cam_pos = Player_Camera_GetPosition();

	Light_SetAmbient({ 1.0f,1.0f,1.0f });//环境光照颜色
	Light_SetDirectionalWorld({ 0.0f,-1.0f,0.0f,0.0f }, { 0.3f,0.3f,0.3f,1.0f });//方向光

	Light_SetSpecularWorld(Player_Camera_GetPosition(), 10.0f, { 0.4f,0.4f,0.4f,1.0f });

	//Light_SetPointLightWorldByCount(0,
	//	{ 0.0f, 2.0f, -3.0f }, // 光源位置 (在模型附近)
	//	 50.0f,                 // 光照范围
	//	{ 1.0f, 1.0f, 1.0f }     // 光源颜色 (白色)
	//);
	Sampler_SetFilterAnisotropic();
	Sky_Draw();
	Map_Draw();
	Player_Draw();
	Enemy_Draw();
	Bullet_Draw();
	BulletHitEffect_Draw();

	Direct3D_SetOffscreenTexture(0);
	Direct3D_SetDepthEnable(false);
	Sprite_Begin();
	float mapW = 300.0f;
	float mapH = 300.0f;
	float mapX = 1920.0f - mapW - 20.0f;
	float mapY = 20.0f;
	Sprite_Draw(Direct3D_GetOffscreenSRV(), mapX, mapY, mapW, mapH);
	Inventory_Draw();
	Direct3D_SetDepthEnable(true);

}

void Game_Finalize()
{
	Sky_Finalize();
	MeshField_Finalize();
	Player_Finalize();
	Enemy_Finalize();
	Bullet_Finalize();
	BulletHitEffect_Finalize();
	Player_Camera_Finalize();
	Map_Finalize();
	Billboard_Finalize();
	Inventory_Finalize();
}


