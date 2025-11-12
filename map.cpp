#include "map.h"
#include "collision.h"
#include "model.h"
#include "cube.h"
#include "texture.h"
#include <DirectXMath.h>
using namespace DirectX;



namespace {
	MapObject g_MapObjects[]{
		{ 0, { 0.0f, 0.0f, 0.0f }, { { -25.0f, -1.0f, -25.0f}, {25.0f, 0.0f, 25.0f}}},
		{1,{ 1.0f,0.5f,0.0f}},
		{1,{-1.0f,0.5f,0.0f}},
		{1,{ 0.0f,0.5f,0.0f}},
		{1,{ 1.0f,0.5f,1.0f}},
		{1,{-1.0f,0.5f,1.0f}},
		{1,{ 1.0f,0.5f,2.0f}},
		{1,{ 0.0f,1.5f,2.0f}},
		{1,{-1.0f,1.5f,2.0f}},
	};

	int g_CubeTexID = -1;
}

void Map_Initialize()
{
	g_CubeTexID = Texture_LoadFromFile(L"resource/texture/Cube_Draw.png");

	for (MapObject& o : g_MapObjects)
	{
		if (o.KindId == 1 || o.KindId == 2) {
			o.Aabb = Cube_CreateAABB(o.Position);
		}
	}
}


void Map_Finalize()
{
}

void Map_Draw()
{
	XMMATRIX mtxWorld;
	for (const MapObject& o : g_MapObjects) {
		switch (o.KindId) {
		case 0:
			break;
		case 1:
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_Draw(g_CubeTexID, mtxWorld);
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
