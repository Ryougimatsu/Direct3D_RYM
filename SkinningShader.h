#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

// 初始化与释放
bool SkinningShader_3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void SkinningShader_3D_Finalize();

// 开始渲染 (绑定Shader)
void SkinningShader_3D_Begin();

// 设置矩阵参数
void SkinningShader_3D_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void SkinningShader_3D_SetViewMatrix(const DirectX::XMMATRIX& matrix);
void SkinningShader_3D_SetProjectMatrix(const DirectX::XMMATRIX& matrix);

// 设置骨骼矩阵数组 (支持两种接口)
void SkinningShader_3D_SetBoneTransforms(const DirectX::XMFLOAT4X4* bones, int count);
void SkinningShader_3D_SetBoneTransforms(const std::vector<DirectX::XMMATRIX>& boneMatrices);
void SkinningShader_3D_SetMaterialColor(const DirectX::XMFLOAT4& color);
// 设置颜色
void SkinningShader_3D_SetColor(const DirectX::XMFLOAT4& color);
void SkinningShader_3D_BeginDepthOnly();
void SkinningShader_3D_SetShadowResources(ID3D11ShaderResourceView* pShadowSRV, const DirectX::XMMATRIX& lightViewProj);
struct ID3D11Buffer;
ID3D11Buffer* SkinningShader_3D_GetBoneBuffer();