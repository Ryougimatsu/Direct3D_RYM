#pragma once
#include <DirectXMath.h>
#include "collision.h"

enum MapKind {
	MAP_KIND_GROUND = 0,
	MAP_KIND_WALL = 1,  // Ïä×Ó/Ç½±Ú
};

class MapObject
{
public:
	int KindId;
	DirectX ::XMFLOAT3 Position;
	AABB Aabb;
};
void Map_Initialize(const DirectX::XMFLOAT3& goalPos);

void Map_Finalize();
//void Map_Update(double elapsed_time);

void Map_Draw();

int Map_GetObjectsCount();

const MapObject* Map_GetObject(int index);

const std::vector<MapObject>& Map_GetObjects();


bool Map_CheckCollision(const AABB& objAabb);

bool Map_CheckLineOfSightBlocked(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end);

