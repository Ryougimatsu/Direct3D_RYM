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


namespace
{
	bool g_IsDebugCameraMode = false;
	SkinningModel* g_TestModel = nullptr;
	Animator g_TestAnimator;
	float g_AnimTime = 0.0f;

	float g_ModelX = 0.0f;
	float g_ModelY = 0.0f;
	float g_ModelZ = 0.0f;



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
	g_TestModel = new SkinningModel();
	if (g_TestModel->Load("resource/model/Throw.fbx", 1.0f)) //
	{
		// [新增] 加载成功后，让播放器开始播放该模型自带的第一个动画
		const Animation& anim = g_TestModel->GetAnimation();
		g_TestAnimator.PlayAnimation(&anim, true); //
	}

	g_ModelX = 10.0f;
	g_ModelY = 0.0f;
	g_ModelZ = 5.0f;
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

	if (g_TestModel)
	{
		g_TestAnimator.Update(elapsed_time); // 自动处理采样时间
	}
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

	if (g_TestModel)
	{
		// 1. 设置着色器通用的 View/Proj 矩阵
		SkinningShader_3D_SetViewMatrix(view);
		SkinningShader_3D_SetProjectMatrix(proj);

		// 2. 计算模型空间变换 (World Matrix)
		float scaleFactor = 0.01f; // 缩小 100 倍
		XMMATRIX mScale = XMMatrixScaling(scaleFactor, scaleFactor, scaleFactor);
		XMMATRIX mRot = XMMatrixRotationY(XM_PI);
		XMMATRIX mTrans = XMMatrixTranslation(g_ModelX, g_ModelY, g_ModelZ);
		XMMATRIX worldMatrix = mScale * mRot * mTrans;

		SkinningShader_3D_SetWorldMatrix(worldMatrix);

		// 3. [关键步骤] 获取当前帧计算好的骨骼矩阵数组并传给 GPU
		// 获取 Final = GlobalAnimated * InvBindPose 的矩阵调色板
		std::vector<XMMATRIX> finalBones = g_TestAnimator.GetFinalBoneMatrices(g_TestModel->GetSkeleton()); //
		SkinningShader_3D_SetBoneTransforms(finalBones); //

		// 4. 开启蒙皮着色器并执行绘制
		SkinningShader_3D_Begin();

		// 遍历模型的所有网格并提交 DrawCall
		 // 5. 遍历模型网格并提交 DrawCall（这里加入材质 / 贴图绑定）
		const auto& meshes = g_TestModel->GetMeshes();
		const auto& materials = g_TestModel->GetMaterials();
		ID3D11DeviceContext* ctx = Direct3D_GetDeviceContext();

		for (const auto& mesh : meshes)
		{
			// --- 5.1 绑定材质贴图 ---
			if (mesh.MaterialIndex < materials.size())
			{
				const SkinningMaterial& mat = materials[mesh.MaterialIndex];

				DirectX::XMFLOAT4 finalColor = mat.DiffuseColor;
				// 如果读取到的颜色是全黑 (0,0,0)，强制设为白色，防止 PS 乘法后结果全黑
				if (finalColor.x == 0.0f && finalColor.y == 0.0f && finalColor.z == 0.0f) {
					finalColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				}
				SkinningShader_3D_SetMaterialColor(finalColor);

				// 如果有 Diffuse 贴图，就绑定到 t0
				if (mat.DiffuseSRV) {
					ctx->PSSetShaderResources(0, 1, &mat.DiffuseSRV);
				}
				else {
					// 重要：如果没有贴图，必须解绑 t0，防止“贴图污染”
					ID3D11ShaderResourceView* nullSRV = nullptr;
					ctx->PSSetShaderResources(0, 1, &nullSRV);
				}
				// 如果你在 PS 里有材质颜色常量缓冲，
				// 这里也可以顺便设置 mat.DiffuseColor（略）
			}

			// --- 5.2 绑定 VB / IB 并绘制 ---
			UINT stride = sizeof(VertexSkinning);
			UINT offset = 0;
			ctx->IASetVertexBuffers(0, 1, &mesh.VertexBuffer, &stride, &offset);
			ctx->IASetIndexBuffer(mesh.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			ctx->DrawIndexed(mesh.IndexCount, 0, 0);
		}
	}
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
	if (g_TestModel)
	{
		g_TestModel->Release();
		delete g_TestModel;
		g_TestModel = nullptr;
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