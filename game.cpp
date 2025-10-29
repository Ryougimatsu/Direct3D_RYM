#include "game.h"
#include "cube.h"
#include "shader.h"
#include "Grid.h"
#include "camera.h"
#include "Meshfield.h"
#include "Light.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "model.h"

namespace 
{
	XMMATRIX g_mtxWorld_Cube;
	XMMATRIX g_mtxWorld_Field;
	MODEL* g_Model = nullptr;
}
void Game_Initialize()
{

	Camera_Initialize({ 0.00, 3.91, -7.80 }, { 0.00, -0.44, 0.90 }, { 1.00, 0.00, 0.00 }, { -0.00, 0.90, 0.44 });
	//g_Model = ModelLoad("resource/Model/NIKKE/Doro.fbx",1.5f);
	g_Model = ModelLoad("resource/Model/test.fbx",0.2f);
}

void Game_Update(double elapsed_time)
{	
	Cube_Update(elapsed_time);
	Camera_Update(elapsed_time);

}

void Game_Draw()
{
	
	Light_SetAmbient({ 0.0f,0.0f,0.0f });//环境光照颜色
	Light_SetDirectionalWorld({ 0.0f,-1.0f,0.0f,0.0f }, { 0.0f,0.0f,0.0f,0.0f });//方向光
	//Grid_Draw();
	XMMATRIX mtxWorld = XMMatrixIdentity();
	XMMATRIX mtxWorldCube = XMMatrixTranslation(3.0f, 0.6f, 0.0f);
	Cube_Draw(mtxWorldCube);

	Light_SetSpecularWorld(Camera_GetPosition(), 1.0f, { 0.1f,0.1f,0.1f,0.1f });//镜面反射光
	MeshField_Draw(mtxWorld);

	Light_SetSpecularWorld(Camera_GetPosition(), 10.0f, { 0.4f,0.4f,0.4f,1.0f });

	Light_SetPointLightWorldByCount(0,
		{ 0.0f, 2.0f, -3.0f }, // 光源位置 (在模型附近)
		 50.0f,                 // 光照范围
		{ 1.0f, 1.0f, 1.0f }     // 光源颜色 (白色)
	);

	XMMATRIX mtxWorldModel = XMMatrixTranslation(-3.0f, 1.6f, 0.0f);
	ModelDraw(g_Model, mtxWorldModel); // 绘制模型
	Debug_Draw();
}

void Game_Finalize()
{
	Cube_Finalize();
	ModelRelease(g_Model);
	MeshField_Finalize();
	Grid_Finalize();
	Camera_Finalize();
}




