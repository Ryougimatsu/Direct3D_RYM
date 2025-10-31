#include "game.h"
#include "shader.h"
//#include "camera.h"
#include "Sampler.h"
#include "Meshfield.h"
#include "Light.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "model.h"
#include "Player.h"
#include "Player_Camera.h"

namespace 
{
	XMMATRIX g_mtxWorld_Field;
	MODEL* g_Model = nullptr;
}
void Game_Initialize()
{
	
	Player_Initialize({ 0.0f, 3.0f, 0.0f }, { 0.0f,0.0f,1.0f });
	Player_Camera_Initialize();
	//Camera_Initialize({ 0.00, 3.91, -7.80 }, { 0.00, -0.44, 0.90 }, { 1.00, 0.00, 0.00 }, { -0.00, 0.90, 0.44 });
	//g_Model = ModelLoad("resource/Model/NIKKE/Doro.fbx",1.5f);
	
}

void Game_Update(double elapsed_time)
{	
	Player_Update(elapsed_time);
	//Camera_Update(elapsed_time);
	Player_Camera_Update(elapsed_time);

}

void Game_Draw()
{
	
	Light_SetAmbient({ 1.0f,1.0f,1.0f });//环境光照颜色
	Light_SetDirectionalWorld({ 0.0f,-1.0f,0.0f,0.0f }, { 0.3f,0.3f,0.3f,1.0f });//方向光
	XMMATRIX mtxWorld = XMMatrixIdentity();

	Light_SetSpecularWorld(Player_Camera_GetPosition(), 1.0f, { 0.1f,0.1f,0.1f,0.1f });//镜面反射光
	MeshField_Draw(mtxWorld);

	Light_SetSpecularWorld(Player_Camera_GetPosition(), 10.0f, { 0.4f,0.4f,0.4f,1.0f });

	//Light_SetPointLightWorldByCount(0,
	//	{ 0.0f, 2.0f, -3.0f }, // 光源位置 (在模型附近)
	//	 50.0f,                 // 光照范围
	//	{ 1.0f, 1.0f, 1.0f }     // 光源颜色 (白色)
	//);
	Sampler_SetFilterAnisotropic();
	Player_Draw();

	//XMMATRIX mtxWorldModel = XMMatrixTranslation(-3.0f, 1.6f, 0.0f);
	//ModelDraw(g_Model, mtxWorldModel); // 绘制模型
	//Debug_Draw();
}

void Game_Finalize()
{
	//ModelRelease(g_Model);
	MeshField_Finalize();
	Player_Finalize();
	Player_Camera_Finalize();
	//Camera_Finalize();
}




