#pragma once
#include <DirectXMath.h>
#include "collision.h"


void Bullet_Initialize();
void Bullet_Finalize();
void Bullet_Update(double elapsed_time);
void Bullet_Draw();

void Bullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity);
void Bullet_Destroy(int index);

int Bullet_GetCount();
AABB Bullet_GetAABB(int index);