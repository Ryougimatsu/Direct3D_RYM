#pragma once
#ifndef SHADER_FIELD_H
#define	SHADER_FIELD_H

#include <d3d11.h>
#include <DirectXMath.h>

bool Shader_field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader_field_Finalize();

void Shader_field_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader_field_3D_SetColor(const DirectX::XMFLOAT4& color);
void Shader_field_Begin();
void Shader_field_SetLightData(const DirectX::XMMATRIX& lightViewProj, ID3D11ShaderResourceView* shadowSRV);

#endif 
