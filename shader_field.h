#pragma once
#ifndef SHADER_FIELD_H
#define	SHADER_FIELD_H

#include <d3d11.h>
#include <DirectXMath.h>

bool Shader_field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader_field_Finalize();

void Shader_field_SetMatrix(const DirectX::XMMATRIX& matrix);
void Shader_field_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader_field_SetViewMatrix(const DirectX::XMMATRIX& matrix);
void Shader_field_SetProjectMatrix(const DirectX::XMMATRIX& matrix);
void Shader_field_Begin();

#endif 
