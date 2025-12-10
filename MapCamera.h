#pragma once
#include <DirectXMath.h>
void MapCam_Initialize();
void MapCam_Finalize();

void MapCam_SetFront(const DirectX::XMFLOAT3& front);
void MapCam_SetPosition(const DirectX::XMFLOAT3& position);

const DirectX::XMFLOAT4X4& MapCam_GetViewMatrix();
const DirectX::XMFLOAT4X4& MapCam_GetPerspectiveMatrix();