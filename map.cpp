#include "map.h"
#include "collision.h"
#include "model.h"
#include "cube.h"
#include "texture.h"
#include <DirectXMath.h>
#include "Light.h"
#include "Meshfield.h"
#include "Player_Camera.h"
#include "shader_3d.h"
using namespace DirectX;



namespace {
	MapObject g_MapObjects[]{
		{ 0, { 0.0f, 0.0f, 0.0f }, { { -25.0f, -1.0f, -12.5f}, {25.0f, 0.0f, 12.5f}}},
		{1,{ 1.0f,0.5f,0.0f}},
		{1,{-1.0f,0.5f,0.0f}},
		{1,{ 0.0f,0.5f,0.0f}},
		{1,{ 1.0f,0.5f,1.0f}},
		{1,{-1.0f,0.5f,1.0f}},
		{1,{ 1.0f,0.5f,2.0f}},
		{1,{ 0.0f,1.5f,2.0f}},
		{1,{-1.0f,1.5f,2.0f}},
		{2,{-1.0f,8.5f,2.0f}},
		{2,{-1.0f,6.5f,2.0f}},
		{2,{-1.0f,5.5f,2.0f}},
		{2,{-1.0f,4.5f,2.0f}},
	};

	int g_CubeTexID = -1;
	MODEL* g_Model = nullptr;
	
}

void Map_Initialize()
{
	g_CubeTexID = Texture_LoadFromFile(L"resource/texture/Cube_Draw.png");
	g_Model = ModelLoad("resource/Model/Tree.fbx", 0.5f);
	for (MapObject& o : g_MapObjects)
	{
		if (o.KindId == 1 || o.KindId == 2) {
			o.Aabb = Cube_CreateAABB(o.Position);
		}
	}
}


void Map_Finalize()
{
	ModelRelease(g_Model);
}

void Map_Draw()
{
	XMMATRIX mtxWorld;

	// 遍历所有地图物体
	for (const MapObject& o : g_MapObjects) {
		switch (o.KindId) {
		case 0: // 地面 (MeshField)
			mtxWorld = XMMatrixIdentity();
			// 开启地面专用的低反光设置
			Light_SetSpecularWorld(Player_Camera_GetPosition(), 1.0f, { 0.1f,0.1f,0.1f,1.0f });
			MeshField_Draw(mtxWorld);

			// 【重要】画完地面后，把高光重置回默认值（比如更亮），否则树木会很暗
			Light_SetSpecularWorld(Player_Camera_GetPosition(), 10.0f, { 0.8f,0.8f,0.8f,1.0f });
			break;

		case 1: // 方块 (Cube)
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_Draw(g_CubeTexID, mtxWorld);
			break;

		case 2: // 树木 (Model) - 【之前这里是空的，现在补全】
		{
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			// 确保模型加载成功再画，防止崩溃
			if (g_Model) {
				ModelDraw(g_Model, mtxWorld);
			}
		}
		break;
		}
	}
}

int Map_GetObjectsCount()
{
	return sizeof(g_MapObjects) / sizeof(g_MapObjects[0]);
}

const MapObject* Map_GetObject(int index)
{
	return &g_MapObjects[index];
}
