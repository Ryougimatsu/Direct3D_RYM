#pragma once
#include <DirectXMath.h>
class MapObject
{
public:
	int KindId;
	DirectX ::XMFLOAT3 Position;

};
void Map_Initialize();

void Map_Finalize();
//void Map_Update(double elapsed_time);

void Map_Draw();

int Map_GetObjectsCount();

const MapObject* Map_GetObject(int index);


