#include "SkinningShader.h"
#include <fstream>
#include <vector>

using namespace DirectX;

#define MAX_BONES 256 

namespace
{
	ID3D11VertexShader* g_pVertexShader = nullptr;
	ID3D11PixelShader* g_pPixelShader = nullptr;
	ID3D11InputLayout* g_pInputLayout = nullptr;

	// 常量缓冲区
	ID3D11Buffer* g_pCBWorld = nullptr; // b0
	ID3D11Buffer* g_pCBView = nullptr; // b1
	ID3D11Buffer* g_pCBProj = nullptr; // b2
	ID3D11Buffer* g_pCBBones = nullptr; // b3 (动态更新优化)
	ID3D11Buffer* g_pCBLightViewProj = nullptr;
	ID3D11ShaderResourceView* g_pShadowSRV_Bound = nullptr;
	ID3D11Buffer* g_pCBColor = nullptr; // PS b0
	ID3D11SamplerState* pShadowSampler = nullptr;
	ID3D11SamplerState* g_pSamplerState = nullptr;

	ID3D11Device* g_pDevice = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;
}

bool SkinningShader_3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) return false;
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 1. 加载 VS 并创建 Input Layout
	std::ifstream ifs_vs("resource/shader/SkinningShader_VS.cso", std::ios::binary);
	if (!ifs_vs) return false;
	std::vector<char> vsData((std::istreambuf_iterator<char>(ifs_vs)), std::istreambuf_iterator<char>());
	g_pDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &g_pVertexShader);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsData.data(), vsData.size(), &g_pInputLayout);

	// 2. 创建常规常量缓冲区 (b0, b1, b2, PS b0)
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.ByteWidth = sizeof(XMMATRIX);
	g_pDevice->CreateBuffer(&bd, nullptr, &g_pCBWorld);
	g_pDevice->CreateBuffer(&bd, nullptr, &g_pCBView);
	g_pDevice->CreateBuffer(&bd, nullptr, &g_pCBProj);
	g_pDevice->CreateBuffer(&bd, nullptr, &g_pCBLightViewProj);
	bd.ByteWidth = sizeof(XMFLOAT4);
	g_pDevice->CreateBuffer(&bd, nullptr, &g_pCBColor);

	// 3. 核心改进：创建动态骨骼缓冲区 (b3)
	D3D11_BUFFER_DESC boneBd = {};
	boneBd.Usage = D3D11_USAGE_DYNAMIC;              // ★ 设为动态
	boneBd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	boneBd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;  // ★ 允许 CPU 写入
	boneBd.ByteWidth = sizeof(XMMATRIX) * MAX_BONES;
	g_pDevice->CreateBuffer(&boneBd, nullptr, &g_pCBBones);

	// 4. 加载 PS 与采样器
	std::ifstream ifs_ps("resource/shader/SkinningShader_PS.cso", std::ios::binary);
	std::vector<char> psData((std::istreambuf_iterator<char>(ifs_ps)), std::istreambuf_iterator<char>());
	g_pDevice->CreatePixelShader(psData.data(), psData.size(), nullptr, &g_pPixelShader);

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 16; // 16倍各向异性过滤
	// 必须设置为 WRAP，否则 Mixamo 某些部位的贴图会出现边缘拉伸
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX; // 允许使用所有的 Mipmap 等级

	HRESULT hrSampler = g_pDevice->CreateSamplerState(&samplerDesc, &g_pSamplerState);
	if (FAILED(hrSampler)) return false;

	// 创建阴影比较采样器 (Shadow Sampler)
	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // 支持 PCF
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f; // 边界外视为无阴影
	shadowSampDesc.BorderColor[1] = 1.0f;
	shadowSampDesc.BorderColor[2] = 1.0f;
	shadowSampDesc.BorderColor[3] = 1.0f;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL; // 关键：深度比较函数

	g_pDevice->CreateSamplerState(&shadowSampDesc, &pShadowSampler);

	return true;
}

// 辅助更新：Map/Unmap 方式
void SkinningShader_3D_SetBoneTransforms(const std::vector<XMMATRIX>& boneMatrices)
{
	if (!g_pContext || !g_pCBBones) return;

	size_t count = boneMatrices.size();
	if (count > MAX_BONES) count = MAX_BONES;

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	if (SUCCEEDED(g_pContext->Map(g_pCBBones, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
	{
		XMMATRIX* pDest = (XMMATRIX*)mappedResource.pData;

		size_t i = 0;
		// 1) 拷贝 Animator 的矩阵
		for (; i < count; ++i)
		{
			pDest[i] = XMMatrixTranspose(boneMatrices[i]);
		}

		// 2) 剩余部分补成单位矩阵
		for (; i < MAX_BONES; ++i)
		{
			pDest[i] = XMMatrixIdentity();
		}

		// 3) 一次 Unmap，结束本次写入
		g_pContext->Unmap(g_pCBBones, 0);
	}
}

// 兼容旧接口
void SkinningShader_3D_SetBoneTransforms(const XMFLOAT4X4* bones, int count)
{
	std::vector<XMMATRIX> matrices;
	for (int i = 0; i < count; ++i) matrices.push_back(XMLoadFloat4x4(&bones[i]));
	SkinningShader_3D_SetBoneTransforms(matrices);
}

void SkinningShader_3D_Begin()
{
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);
	g_pContext->IASetInputLayout(g_pInputLayout);

	ID3D11Buffer* vsBuffers[] = { g_pCBWorld, g_pCBView, g_pCBProj, g_pCBBones };
	g_pContext->VSSetConstantBuffers(0, 4, vsBuffers);
	g_pContext->PSSetConstantBuffers(0, 1, &g_pCBColor);
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);
}

// 其他矩阵设置函数保持 UpdateSubresource 即可 (因为每帧只更新一次)
void SkinningShader_3D_SetWorldMatrix(const XMMATRIX& m) {
	XMMATRIX mt = XMMatrixTranspose(m);
	g_pContext->UpdateSubresource(g_pCBWorld, 0, nullptr, &mt, 0, 0);
}

void SkinningShader_3D_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
	XMMATRIX mt = XMMatrixTranspose(matrix);
	g_pContext->UpdateSubresource(g_pCBView, 0, nullptr, &mt, 0, 0);
}

void SkinningShader_3D_SetProjectMatrix(const DirectX::XMMATRIX& matrix)
{
	XMMATRIX mt = XMMatrixTranspose(matrix);
	g_pContext->UpdateSubresource(g_pCBProj, 0, nullptr, &mt, 0, 0);
}
void SkinningShader_3D_SetMaterialColor(const DirectX::XMFLOAT4& color)
{
	if (!g_pContext || !g_pCBColor) return;

	// 将颜色数据更新到 PS 的常量缓冲区 b0
	g_pContext->UpdateSubresource(g_pCBColor, 0, nullptr, &color, 0, 0);
}

void SkinningShader_3D_BeginDepthOnly()
{
	// 1. 绑定 VS 和 InputLayout (复用蒙皮逻辑)
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->IASetInputLayout(g_pInputLayout);

	// 2. 绑定骨骼变换等 VS 常量
	ID3D11Buffer* vsBuffers[] = { g_pCBWorld, g_pCBView, g_pCBProj, g_pCBBones };
	g_pContext->VSSetConstantBuffers(0, 4, vsBuffers);

	// 3. 【关键】解绑 PS，因为生成阴影只需要深度
	g_pContext->PSSetShader(nullptr, nullptr, 0);
}

void SkinningShader_3D_SetShadowResources(ID3D11ShaderResourceView* pShadowSRV, const DirectX::XMMATRIX& lightViewProj)
{
	// 传入阴影图到 PS (假设在 slot 1, slot 0 是漫反射贴图)
	g_pContext->PSSetShaderResources(1, 1, &pShadowSRV);
	g_pContext->PSSetSamplers(1, 1, &pShadowSampler);

	// 传入光源矩阵到 VS 或 PS (取决于你在哪里计算阴影坐标，推荐 VS)
	DirectX::XMMATRIX mT = DirectX::XMMatrixTranspose(lightViewProj);
	g_pContext->UpdateSubresource(g_pCBLightViewProj, 0, nullptr, &mT, 0, 0);
	g_pContext->VSSetConstantBuffers(4, 1, &g_pCBLightViewProj); // 假设绑定到 b4
}

void SkinningShader_3D_Finalize()
{
	if (g_pSamplerState) g_pSamplerState->Release();
	if (g_pCBColor) g_pCBColor->Release();
	if (g_pCBBones) g_pCBBones->Release();
	if (g_pCBProj) g_pCBProj->Release();
	if (g_pCBView) g_pCBView->Release();
	if (g_pCBWorld) g_pCBWorld->Release();
	if (g_pInputLayout) g_pInputLayout->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pVertexShader) g_pVertexShader->Release();

}