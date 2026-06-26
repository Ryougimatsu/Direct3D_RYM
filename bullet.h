#pragma once
#include <DirectXMath.h>
#include "collision.h"

constexpr float BULLET_DEFAULT_DAMAGE = 10.0f;
constexpr int BULLET_DEFAULT_REMAINING_PIERCE = 0;

void Bullet_Initialize();
void Bullet_Finalize();
void Bullet_Update(double elapsed_time);
void Bullet_CheckCollisionWithEnemies();
void Bullet_Draw();

void Bullet_Create(
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& velocity,
	float damage = BULLET_DEFAULT_DAMAGE,
	int remainingPierce = BULLET_DEFAULT_REMAINING_PIERCE);
void Bullet_Destroy(int index);

int Bullet_GetCount();
Sphere Bullet_GetSphere(int index);
AABB Bullet_GetAABB(int index);
