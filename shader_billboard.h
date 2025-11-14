/*==============================================================================

   Billboarｄ用[Shader_3D.h]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
struct UVParameter
{
	DirectX ::XMFLOAT2 scale;
	DirectX ::XMFLOAT2 translation;
};

bool Shader_Billboard_Initialize();
void Shader_Billboard_Finalize();

void Shader_Billboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader_Billboard_SetViewMatrix(const DirectX::XMMATRIX& matrix);
void Shader_Billboard_SetProjectMatrix(const DirectX::XMMATRIX& matrix);
void Shader_Billboard_SetColor(const DirectX::XMFLOAT4& color);
void Shader_Billboard_SetUVParameter(const UVParameter& parameter);
void Shader_Billboard_Begin();
