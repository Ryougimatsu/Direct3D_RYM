#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "shader_3d.h"

#define MAX_BONES 100
namespace
{
	ID3D11VertexShader* g_pVertexShader = nullptr;
	ID3D11InputLayout* g_pInputLayout = nullptr;

	// 常量缓冲区
	ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // World Matrix (b0)
	ID3D11Buffer* g_pCB_LightViewProj = nullptr; // Light Matrix (b3)
	ID3D11Buffer* g_pPSConstantBuffer0 = nullptr; // Material Color (b0)

	ID3D11PixelShader* g_pPixelShader = nullptr;

	// 采样器
	ID3D11SamplerState* g_pSamplerState = nullptr; // Slot 0: 普通贴图 (Diffuse)
	ID3D11SamplerState* g_pShadowSampler = nullptr; // Slot 1: 阴影贴图 (Shadow)

	// 注意！初期化で外部から設定されるもの。Release不要。
	ID3D11Device* g_pDevice = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;
}

bool Shader_3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr;

	if (!pDevice || !pContext) return false;
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 1. 读取并创建顶点着色器 (VS)
	std::ifstream ifs_vs("resource/shader/shader_vertex_3d.cso", std::ios::binary);
	if (!ifs_vs) return false;

	ifs_vs.seekg(0, std::ios::end);
	std::streamsize filesize = ifs_vs.tellg();
	ifs_vs.seekg(0, std::ios::beg);

	unsigned char* vsbinary_pointer = new unsigned char[filesize];
	ifs_vs.read((char*)vsbinary_pointer, filesize);
	ifs_vs.close();

	hr = g_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);
	if (FAILED(hr)) { delete[] vsbinary_pointer; return false; }

	// 2. 创建输入布局 (Input Layout)
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsbinary_pointer, filesize, &g_pInputLayout);
	delete[] vsbinary_pointer;
	if (FAILED(hr)) return false;

	// 3. 创建常量缓冲区
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	// VS Constant Buffer 0 (World Matrix)
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0);

	// VS Constant Buffer 3 (Light ViewProj)
	hr = g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pCB_LightViewProj);
	if (FAILED(hr)) return false;

	// PS Constant Buffer 0 (Material Color)
	buffer_desc.ByteWidth = sizeof(XMFLOAT4);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0);

	// 4. 读取并创建像素着色器 (PS)
	std::ifstream ifs_ps("resource/shader/shader_pixel_3d.cso", std::ios::binary);
	if (!ifs_ps) return false;

	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	hr = g_pDevice->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);
	delete[] psbinary_pointer;
	if (FAILED(hr)) return false;

	// 5. 创建采样器 (Sampler States)

	// (A) 普通贴图采样器 (Slot 0) - Linear Wrap
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	g_pDevice->CreateSamplerState(&sampDesc, &g_pSamplerState);

	// (B) 阴影贴图采样器 (Slot 1) - Comparison Border
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS; // 深度比较：如果 当前深度 < 阴影图深度，则照亮

	// 【关键调试点】
	// 如果设为 1.0f (白色)，采样到贴图外时是“无阴影”。
	// 如果设为 0.0f (黑色)，采样到贴图外时是“全黑”。
	sampDesc.BorderColor[0] = 1.0f;
	sampDesc.BorderColor[1] = 1.0f;
	sampDesc.BorderColor[2] = 1.0f;
	sampDesc.BorderColor[3] = 1.0f;

	hr = g_pDevice->CreateSamplerState(&sampDesc, &g_pShadowSampler);
	if (FAILED(hr)) return false;

	return true;
}

void Shader_3D_Finalize()
{
	SAFE_RELEASE(g_pCB_LightViewProj);
	SAFE_RELEASE(g_pShadowSampler);
	SAFE_RELEASE(g_pSamplerState); // 释放新增的采样器
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pPSConstantBuffer0);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
}

void Shader_3D_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	XMFLOAT4X4 transpose;
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));
	g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void Shader_3D_SetColor(const XMFLOAT4& color)
{
	g_pContext->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void Shader_3D_Begin()
{
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);
	g_pContext->IASetInputLayout(g_pInputLayout);
	g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
	if (g_pCB_LightViewProj) {
		g_pContext->VSSetConstantBuffers(3, 1, &g_pCB_LightViewProj);
	}

	g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

	g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);
	g_pContext->PSSetSamplers(5, 1, &g_pShadowSampler);
}

void Shader_3D_SetLightData(const XMMATRIX& lightViewProj, ID3D11ShaderResourceView* shadowSRV)
{
	// 1) 可选：先解绑（防止同资源冲突/状态残留）
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	g_pContext->PSSetShaderResources(5, 1, nullSRV);
	g_pContext->PSSetShaderResources(1, 1, nullSRV);

	g_pContext->PSSetShaderResources(5, 1, &shadowSRV);

	g_pContext->PSSetSamplers(5, 1, &g_pShadowSampler);

	XMFLOAT4X4 transpose;
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(lightViewProj));
	g_pContext->UpdateSubresource(g_pCB_LightViewProj, 0, nullptr, &transpose, 0, 0);
	g_pContext->VSSetConstantBuffers(3, 1, &g_pCB_LightViewProj);
}
