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

namespace
{
	bool g_IsDebugCameraMode = false;
	PlayerCharacter* g_Player = nullptr;
	float g_AnimTime = 0.0f;
	struct SkinnedEntity {
		SkinningModel* Model = nullptr;
		Animator EntityAnimator;
		XMFLOAT3 Position = { 0, 0, 0 };
		float Scale = 0.01f;

		void Update(double dt) {
			if (Model) EntityAnimator.Update(dt);
		}

		void Render(const XMMATRIX& view, const XMMATRIX& proj) {
			if (!Model) return;
			// 1. 设置着色器全局矩阵
			SkinningShader_3D_SetViewMatrix(view);
			SkinningShader_3D_SetProjectMatrix(proj);

			// 2. 设置当前物体的世界矩阵
			XMMATRIX mScale = XMMatrixScaling(Scale, Scale, Scale);
			XMMATRIX mRot = XMMatrixRotationY(XM_PI);
			XMMATRIX mTrans = XMMatrixTranslation(Position.x, Position.y, Position.z);
			SkinningShader_3D_SetWorldMatrix(mScale * mRot * mTrans);

			// 3. 传输当前帧骨骼矩阵
			std::vector<XMMATRIX> finalBones = EntityAnimator.GetFinalBoneMatrices(Model->GetSkeleton());
			SkinningShader_3D_SetBoneTransforms(finalBones);

			// 4. 开始绘制底层
			SkinningShader_3D_Begin();
			Model->Draw(); // 这里调用了你刚才重构在 SkinningModel 里的 Draw 函数
		}
	} g_TestEntity;

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
	DropItem_Draw();
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
	Enemy_Create({ -3.0f,1.0f,5.0f });
	Inventory_Initialize();
	DropItem_Initialize();
	GameUI_Initialize();
	g_TestEntity.Model = new SkinningModel();
	if (g_TestEntity.Model->Load("resource/model/Idle.fbx", 1.0f))
	{
		// 获取默认动画并播放
		const Animation* anim = g_TestEntity.Model->GetDefaultAnimation();
		g_TestEntity.EntityAnimator.PlayAnimation(anim, true);
	}

	g_TestEntity.Position = { 10.0f, 0.0f, 5.0f };
	g_TestEntity.Scale = 0.01f;
	g_Player = new PlayerCharacter();
	g_Player->Initialize();
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
	DropItem_Update(elapsed_time);

	if (KeyLogger_IsTrigger(KK_TAB))
	{
		g_IsDebugCameraMode = !g_IsDebugCameraMode;

		if (g_IsDebugCameraMode)
		{
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
			DebugCamera_SetPosition(Player_Camera_GetPosition());
		}
		else
		{
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
		const MapObject* pMapObj = Map_GetObject(j);
		for (int i = 0; i < Bullet_GetCount(); i++)
		{
			AABB bulletAABB = Bullet_GetAABB(i);
			AABB mapObjAABB = pMapObj->Aabb;

			if (Collision_IsOverLapAABB(bulletAABB, mapObjAABB))
			{
				BulletHitEffect_Create(Bullet_GetSphere(i).center);
				Bullet_Destroy(i);
				break;
			}
		}
	}
	for (int j = 0; j < Enemy_GetEnemyCount(); j++)
	{
		Enemy* pEnemy = Enemy_GetEnemy(j);
		if (pEnemy == nullptr || pEnemy->IsDestroyed()) continue;

		AABB enemyAABB = pEnemy->GetAABB();

		for (int i = 0; i < Bullet_GetCount(); i++)
		{
			if (!Bullet_GetSphere(i).radius > 0) continue;

			Sphere bulletSphere = Bullet_GetSphere(i);

			if (Collision_IsOverlapSphereAABB(bulletSphere, enemyAABB))
			{
				BulletHitEffect_Create(bulletSphere.center);
				pEnemy->Damage(10.0f);
				if (pEnemy->IsDestroyed())
				{
					XMFLOAT3 enemyPos = pEnemy->GetPosition();
					DropItem_Spawn(enemyPos, 0);
				}
				Bullet_Destroy(i);
				break;
			}
		}
	}

	g_TestEntity.Update(elapsed_time);
	if (g_Player) g_Player->Update(elapsed_time);
}

void Game_Draw()
{

	Game_RenderMiniMap();
	Direct3D_SetOffBackBuffer();
	Direct3D_ClearBackBuffer();

	XMMATRIX view, proj;
	XMFLOAT3 camPos;
	if (g_IsDebugCameraMode)
	{
		XMFLOAT4X4 v = DebugCamera_GetViewMatrix();
		XMFLOAT4X4 p = DebugCamera_GetProjectionMatrix();
		view = XMLoadFloat4x4(&v);
		proj = XMLoadFloat4x4(&p);
		camPos = DebugCamera_GetPosition();
	}
	else
	{
		XMFLOAT4X4 v = Player_Camera_GetViewMatrix();
		XMFLOAT4X4 p = Player_Camera_GetProjectionMatrix();
		view = XMLoadFloat4x4(&v);
		proj = XMLoadFloat4x4(&p);
		camPos = Player_Camera_GetPosition();
	}

	g_TestEntity.Render(view, proj);
	if (g_Player) g_Player->Draw(view, proj);
	Shader_3D_Begin();
	Camera_SetMatrixToShader(view, proj);
	XMMATRIX mtxWorld = XMMatrixIdentity();

	XMFLOAT3 cam_pos = Player_Camera_GetPosition();

	Light_SetAmbient({ 1.0f,1.0f,1.0f });
	Light_SetDirectionalWorld({ 0.0f,-1.0f,0.0f,0.0f }, { 0.3f,0.3f,0.3f,1.0f });
	Light_SetSpecularWorld(Player_Camera_GetPosition(), 10.0f, { 0.4f,0.4f,0.4f,1.0f });

	Sampler_SetFilterAnisotropic();
	Sky_Draw();
	Map_Draw();
	Player_Draw();
	Enemy_Draw();
	Bullet_Draw();
	BulletHitEffect_Draw();
	DropItem_Draw();

	Direct3D_SetOffscreenTexture(0);
	Direct3D_SetDepthEnable(false);
	Sprite_Begin();
	float mapW = 300.0f;
	float mapH = 300.0f;
	float mapX = 1920.0f - mapW - 20.0f;
	float mapY = 20.0f;
	Sprite_Draw(Direct3D_GetOffscreenSRV(), mapX, mapY, mapW, mapH);
	Inventory_Draw();
	GameUI_Draw();
	Direct3D_SetDepthEnable(true);
}


void Game_Finalize()
{
	if (g_TestEntity.Model)
	{
		g_TestEntity.Model->Release();
		delete g_TestEntity.Model;
		g_TestEntity.Model = nullptr;
	}
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
	DropItem_Finalize();
}