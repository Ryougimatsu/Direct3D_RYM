#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

bool Shader3DUnilt_Initialize();
void Shader3DUnilt_Finalize();


void Shader3DUnilt_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader3DUnilt_SetViewMatrix(const DirectX::XMMATRIX& matrix);
void Shader3DUnilt_SetProjectMatrix(const DirectX::XMMATRIX& matrix);
void Shader3DUnilt_SetColor(const DirectX::XMFLOAT4& color);
void Shader3DUnilt_Begin();