#ifndef SHADER_SHADOW_H
#define SHADER_SHADOW_H

#include <d3d11.h>
#include <DirectXMath.h>

// 初始化阴影资源 (Texture, DSV, SRV)
bool Shader_Shadow_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader_Shadow_Finalize();

// 开始阴影生成 Pass (渲染到深度图)
void Shader_Shadow_Begin(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj);

// 在 Vertex Shader 中设置 World 矩阵 (用于 Pass 1)
void Shader_Shadow_SetWorldMatrix(const DirectX::XMMATRIX& worldMatrix);

// 结束阴影 Pass (恢复 BackBuffer)
void Shader_Shadow_End();

// 获取阴影图的 SRV (供 Shader_3D 使用)
ID3D11ShaderResourceView* Shader_Shadow_GetSRV();
void Shader_Shadow_Apply();
void Shader_Shadow_ApplySkinning();
#endif