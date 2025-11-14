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
	for (const MapObject& o : g_MapObjects) {
		if (o.KindId == 0)
		{
			// 地面 AABB 画绿色
			Collision_DebugDraw(o.Aabb, { 0.0f, 1.0f, 0.0f, 1.0f });
		}
		else
		{
			// 方块 AABB 画蓝色
			Collision_DebugDraw(o.Aabb, { 0.0f, 0.0f, 1.0f, 1.0f });
		}
		switch (o.KindId) {
		case 0:
			mtxWorld = XMMatrixIdentity(); // 地面は原点に
			Light_SetSpecularWorld(Player_Camera_GetPosition(), 1.0f, { 0.1f,0.1f,0.1f,0.1f }); // 地面用の光沢設定
			MeshField_Draw(mtxWorld);
			break;
		case 1:
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_Draw(g_CubeTexID, mtxWorld);
			break;
		case 2:
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
