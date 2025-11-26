#include "game.h"
#include "shader.h"
#include "Sampler.h"
#include "Meshfield.h"
#include "Light.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "model.h"
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


namespace 
{
	XMMATRIX g_mtxWorld_Field;
	int g_TexTest = -1;
}
void Game_Initialize()
{
	Sky_Initialize();
	Bullet_Initialize();
	BulletHitEffect_Initialize();
	Player_Initialize({ 0.0f, 3.0f, 0.0f }, { 0.0f,0.0f,1.0f });
	Enemy_Initialize();
	Player_Camera_Initialize();
	Map_Initialize();
	Billboard_Initialize();
	
	Enemy_Create({-3.0f,1.0f,5.0f});
}

void Game_Update(double elapsed_time)
{	
	Player_Update(elapsed_time);
	Enemy_Update(elapsed_time);
	Sky_SetPosition(Player_GetPosition());
	Player_Camera_Update(elapsed_time);
	Bullet_Update(elapsed_time);
	BulletHitEffect_Update();

	for (int j = 0; j < Map_GetObjectsCount(); j++)
	{
		for (int i = 0; i < Bullet_GetCount(); i++)
		{
			AABB bullet = Bullet_GetAABB(i);
			AABB mapObj = Map_GetObject(j)->Aabb;
			if (Collision_IsOverLapAABB(bullet, mapObj))
			{
				Bullet_Destroy(i);
				break;
			}
		}
	}

}

void Game_Draw()
{
	
	Light_SetAmbient({ 1.0f,1.0f,1.0f });//环境光照颜色
	Light_SetDirectionalWorld({ 0.0f,-1.0f,0.0f,0.0f }, { 0.3f,0.3f,0.3f,1.0f });//方向光
	XMMATRIX mtxWorld = XMMatrixIdentity();

	Light_SetSpecularWorld(Player_Camera_GetPosition(), 10.0f, { 0.4f,0.4f,0.4f,1.0f });

	//Light_SetPointLightWorldByCount(0,
	//	{ 0.0f, 2.0f, -3.0f }, // 光源位置 (在模型附近)
	//	 50.0f,                 // 光照范围
	//	{ 1.0f, 1.0f, 1.0f }     // 光源颜色 (白色)
	//);
	Sampler_SetFilterAnisotropic();
	Sky_Draw();
	Player_Draw();
	Enemy_Draw();
	Map_Draw();
	Bullet_Draw();
	BulletHitEffect_Draw();

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
}




