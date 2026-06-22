#pragma once

#include <DirectXMath.h>

void Camera_Initialize();
void Camera_Finalize();
void Camera_SetMatrixToShader(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);
