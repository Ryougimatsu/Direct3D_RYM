#include "game.h"
#include "cube.h"
#include "shader.h"
#include "Grid.h"
#include "camera.h"
#include "Meshfield.h"
#include "Light.h"
#include <DirectXMath.h>
using namespace DirectX;

namespace 
{
	XMMATRIX g_mtxWorld_Cube;
	XMMATRIX g_mtxWorld_Field;
}
void Game_Initialize()
{

	Camera_Initialize({ 0.00, 3.91, -7.80 }, { 0.00, -0.44, 0.90 }, { 1.00, 0.00, 0.00 }, { -0.00, 0.90, 0.44 });

}

void Game_Update(double elapsed_time)
{	
	Cube_Update(elapsed_time);
	Camera_Update(elapsed_time);

}

void Game_Draw()
{
	
	Light_SetAmbient({ 0.3f,0.3f,0.3f});//环境光照颜色
	Light_SetDirectionalWorld({ 0.0f,-1.0f,0.0f,0.0f }, { 0.4f,0.4f,0.4f,1.0f });//方向光
	//Grid_Draw();
	XMMATRIX mtxWorld = XMMatrixIdentity();
	Cube_Draw(mtxWorld);
	MeshField_Draw(mtxWorld);
	Debug_Draw();
}

void Game_Finalize()
{
	Cube_Finalize();
	Grid_Finalize();
	Camera_Finalize();
}




