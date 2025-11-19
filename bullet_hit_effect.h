#pragma once
#include <DirectXMath.h>

void BulletHitEffect_Initialize();
void BulletHitEffect_Finalize();
void BulletHitEffect_Update();
void BulletHitEffect_Create(const DirectX::XMFLOAT3& position);
void BulletHitEffect_Draw();