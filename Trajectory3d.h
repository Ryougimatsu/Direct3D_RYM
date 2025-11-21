#pragma once
#include <DirectXMath.h>
void Trajectory3D_Initialize(void);
void Trajectory3D_Finalize(void);

void Trajectory3D_Update(double elapsed_time);
void Trajectory3D_Draw();
	
void Trajectory3D_Create(const DirectX::XMFLOAT3 position,
	const DirectX::XMFLOAT4 color,
	float size,
	double lifetime
	);