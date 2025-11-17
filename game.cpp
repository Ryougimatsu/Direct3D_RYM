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
#include "map.h"
#include "billboard.h"
#include "texture.h"


namespace 
{
	XMMATRIX g_mtxWorld_Field;
	MODEL* g_Model = nullptr;
	int g_TexTest = -1;
}
void Game_Initialize()
{
	
	Player_Initialize({ 0.0f, 3.0f, 0.0f }, { 0.0f,0.0f,1.0f });
	Player_Camera_Initialize();
	g_Model = ModelLoad("resource/Model/Tree.fbx",0.5f);
	Map_Initialize();
	Billboard_Initialize();

	g_TexTest = Texture_LoadFromFile(L"resource/texture/pl00.png");
	
}

void Game_Update(double elapsed_time)
{	
	Player_Update(elapsed_time);
	Player_Camera_Update(elapsed_time);

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
	Player_Draw();

	Map_Draw();

	XMMATRIX mtxWorldModel = XMMatrixTranslation(-3.0f, 0.0f, 0.0f);
	ModelDraw(g_Model, mtxWorldModel); // 绘制模型
	ModelDraw(g_Model, XMMatrixTranslation(3.0f, 0.0f, 2.0f)); // 绘制模型
	ModelDraw(g_Model, XMMatrixTranslation(6.0f, 0.0f, 2.0f));
	ModelDraw(g_Model, XMMatrixTranslation(6.0f, 0.0f, 3.0f));
	ModelDraw(g_Model, XMMatrixTranslation(6.0f, 0.0f, 4.0f));
	ModelDraw(g_Model, XMMatrixTranslation(6.0f, 0.0f, 4.0f));
	ModelDraw(g_Model, XMMatrixTranslation(7.0f, 0.0f, 4.0f));
	ModelDraw(g_Model, XMMatrixTranslation(8.0f, 0.0f, 4.0f));
	ModelDraw(g_Model, XMMatrixTranslation(8.0f, 0.0f, 4.0f));
	ModelDraw(g_Model, XMMatrixTranslation(9.0f, 0.0f, 4.0f));
	ModelDraw(g_Model, XMMatrixTranslation(9.0f, 0.0f, 4.0f));
	ModelDraw(g_Model, XMMatrixTranslation(9.0f, 0.0f, 6.0f));
	ModelDraw(g_Model, XMMatrixTranslation(9.0f, 0.0f, 7.0f));
	ModelDraw(g_Model, XMMatrixTranslation(-6.0f, 0.0f, 2.0f));
	ModelDraw(g_Model, XMMatrixTranslation(-2.0f, 0.0f, 3.0f));
	ModelDraw(g_Model, XMMatrixTranslation(-7.0f, 0.0f, 5.0f));

	Billboard_Draw(g_TexTest, { -10.0f,0.5f,-5.0f }, { 4.0f, 4.0f }, { 0.0f,-0.3 });
}

void Game_Finalize()
{
	ModelRelease(g_Model);
	MeshField_Finalize();
	Player_Finalize();
	Player_Camera_Finalize();
	Map_Finalize();
	Billboard_Finalize();
}




