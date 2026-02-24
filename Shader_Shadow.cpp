#include "Shader_Shadow.h"
#include "debug_ostream.h"
#include <fstream>
#include <vector>

namespace {
	ID3D11Device* g_pDevice = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;

	ID3D11VertexShader* g_pShadowVS = nullptr;
	ID3D11InputLayout* g_pShadowInputLayout = nullptr;

	ID3D11VertexShader* g_pSkinningShadowVS = nullptr;
	ID3D11InputLayout* g_pSkinningShadowInputLayout = nullptr;

	ID3D11Buffer* g_pShadowConstantBuffer = nullptr; // 存放 World * LightView * LightProj

	// 阴影纹理资源
	ID3D11Texture2D* g_pShadowMapTexture = nullptr;
	ID3D11DepthStencilView* g_pShadowDSV = nullptr;
	ID3D11ShaderResourceView* g_pShadowSRV = nullptr;
	ID3D11RasterizerState* g_pShadowRasterizer = nullptr; // 需要 DepthBias



	// 视口
	D3D11_VIEWPORT g_ShadowViewport;
	const float SHADOW_MAP_SIZE = 8192.0f;

	// 缓存的光源矩阵
	DirectX::XMFLOAT4X4 g_LightViewProj;

	// 备份原来的 Viewport 和 RenderTarget
	D3D11_VIEWPORT g_OldViewport;
	ID3D11RenderTargetView* g_pOldRTV = nullptr;
	ID3D11DepthStencilView* g_pOldDSV = nullptr;
}

bool Shader_Shadow_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;
	HRESULT hr;

	// 1. 创建阴影纹理 (Typeless，因为既要作为 Depth 也要作为 Resource)
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = (UINT)SHADOW_MAP_SIZE;
	texDesc.Height = (UINT)SHADOW_MAP_SIZE;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // 关键
	texDesc.SampleDesc.Count = 1;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	hr = g_pDevice->CreateTexture2D(&texDesc, nullptr, &g_pShadowMapTexture);
	if (FAILED(hr)) return false;

	// 2. 创建 DSV (Depth Stencil View) - 用于 Pass 1
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	hr = g_pDevice->CreateDepthStencilView(g_pShadowMapTexture, &dsvDesc, &g_pShadowDSV);
	if (FAILED(hr)) return false;

	// 3. 创建 SRV (Shader Resource View) - 用于 Pass 2
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	hr = g_pDevice->CreateShaderResourceView(g_pShadowMapTexture, &srvDesc, &g_pShadowSRV);
	if (FAILED(hr)) return false;

	// 4. 创建光栅化状态 (增加 Bias 防止波纹)
	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 500;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.SlopeScaledDepthBias = 1.0f;
	hr = g_pDevice->CreateRasterizerState(&rasterDesc, &g_pShadowRasterizer);

	// 5. 设置阴影视口
	g_ShadowViewport.Width = SHADOW_MAP_SIZE;
	g_ShadowViewport.Height = SHADOW_MAP_SIZE;
	g_ShadowViewport.MinDepth = 0.0f;
	g_ShadowViewport.MaxDepth = 1.0f;
	g_ShadowViewport.TopLeftX = 0.0f;
	g_ShadowViewport.TopLeftY = 0.0f;

	// 6. 加载 Shadow Vertex Shader
	// 注意：请确保你编译了 ShadowMap_VS.hlsl 到 cso
	std::ifstream ifs("resource/shader/ShadowMap_VS.cso", std::ios::binary);
	if (!ifs) return false;
	ifs.seekg(0, std::ios::end);
	size_t size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);
	char* binary = new char[size];
	ifs.read(binary, size);
	ifs.close();

	hr = g_pDevice->CreateVertexShader(binary, size, nullptr, &g_pShadowVS);

	// 7. Input Layout (仅需要 POSITION)
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	g_pDevice->CreateInputLayout(layout, 1, binary, size, &g_pShadowInputLayout);
	delete[] binary;

	std::ifstream ifsSkin("resource/shader/SkinningShadowMap_VS.cso", std::ios::binary);
	if (ifsSkin) {
		ifsSkin.seekg(0, std::ios::end);
		size_t sizeSkin = ifsSkin.tellg();
		ifsSkin.seekg(0, std::ios::beg);
		char* binarySkin = new char[sizeSkin];
		ifsSkin.read(binarySkin, sizeSkin);
		ifsSkin.close();

		hr = g_pDevice->CreateVertexShader(binarySkin, sizeSkin, nullptr, &g_pSkinningShadowVS);

		// [注意] 这里的 Layout 必须与你的 C++ VertexSkinning 结构体完全对应！
		// 使用 D3D11_APPEND_ALIGNED_ELEMENT 可以自动计算偏移量
		D3D11_INPUT_ELEMENT_DESC skinLayout[] = {
			{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		g_pDevice->CreateInputLayout(skinLayout, ARRAYSIZE(skinLayout), binarySkin, sizeSkin, &g_pSkinningShadowInputLayout);
		delete[] binarySkin;
	}

	// 8. Constant Buffer (Matrix)
	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_pDevice->CreateBuffer(&cbDesc, nullptr, &g_pShadowConstantBuffer);

	return true;
}

void Shader_Shadow_Begin(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj)
{
	// 保存当前的光源 ViewProj 矩阵
	DirectX::XMStoreFloat4x4(&g_LightViewProj, DirectX::XMMatrixTranspose(lightView * lightProj));

	// 1. 备份当前的 Viewport 和 RenderTargets
	UINT num = 1;
	g_pContext->RSGetViewports(&num, &g_OldViewport);
	g_pContext->OMGetRenderTargets(1, &g_pOldRTV, &g_pOldDSV);
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	g_pContext->PSSetShaderResources(1, 1, nullSRV); // 释放 SkinningShader 残留的 Slot 1
	g_pContext->PSSetShaderResources(5, 1, nullSRV); // 释放 Shader_3D 残留的 Slot 5

	// 2. 设置为 Shadow Map 渲染目标 (No Color Buffer, Only Depth)
	ID3D11RenderTargetView* nullRTV[1] = { nullptr };
	g_pContext->OMSetRenderTargets(1, nullRTV, g_pShadowDSV);
	g_pContext->ClearDepthStencilView(g_pShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// 3. 设置视口
	g_pContext->RSSetViewports(1, &g_ShadowViewport);
	g_pContext->RSSetState(g_pShadowRasterizer);

	// 4. 设置 Shader
	g_pContext->VSSetShader(g_pShadowVS, nullptr, 0);
	g_pContext->PSSetShader(nullptr, nullptr, 0); // 不需要 Pixel Shader
	g_pContext->IASetInputLayout(g_pShadowInputLayout);
	g_pContext->VSSetConstantBuffers(0, 1, &g_pShadowConstantBuffer);
}

void Shader_Shadow_SetWorldMatrix(const DirectX::XMMATRIX& worldMatrix)
{
	using namespace DirectX;
	// 计算 World * LightView * LightProj
	XMMATRIX vp = XMLoadFloat4x4(&g_LightViewProj); // 已经是转置过的 ViewProj
	XMMATRIX w = XMMatrixTranspose(worldMatrix);    // 转置 World

	// 注意：g_LightViewProj 已经转置过了，所以这里计算要注意顺序
	// HLSL: mul(pos, World * VP) -> 这里的 Buffer 应该是 Transpose(World * VP)

	// 重新计算未转置的 VP 以便组合
	XMMATRIX rawVP = XMMatrixTranspose(vp);
	XMMATRIX wvp = worldMatrix * rawVP;

	XMFLOAT4X4 finalCB;
	XMStoreFloat4x4(&finalCB, XMMatrixTranspose(wvp));

	g_pContext->UpdateSubresource(g_pShadowConstantBuffer, 0, nullptr, &finalCB, 0, 0);
}

void Shader_Shadow_End()
{
	// 恢复渲染目标和视口
	g_pContext->OMSetRenderTargets(1, &g_pOldRTV, g_pOldDSV);
	g_pContext->RSSetViewports(1, &g_OldViewport);

	// 恢复默认 Rasterizer (通常是 null 或默认状态)
	g_pContext->RSSetState(nullptr);

	// 释放引用
	if (g_pOldRTV) g_pOldRTV->Release();
	if (g_pOldDSV) g_pOldDSV->Release();
}

ID3D11ShaderResourceView* Shader_Shadow_GetSRV()
{
	return g_pShadowSRV;
}

void Shader_Shadow_Apply()
{
	// 恢复 Shadow Vertex Shader
	g_pContext->VSSetShader(g_pShadowVS, nullptr, 0);
	g_pContext->PSSetShader(nullptr, nullptr, 0); // 阴影生成不需要 PS

	// 恢复 Input Layout
	g_pContext->IASetInputLayout(g_pShadowInputLayout);

	// 恢复 Constant Buffer (绑定到 b0)
	g_pContext->VSSetConstantBuffers(0, 1, &g_pShadowConstantBuffer);

	// 恢复 Rasterizer (带 DepthBias)
	g_pContext->RSSetState(g_pShadowRasterizer);
}

void Shader_Shadow_ApplySkinning()
{
	// 绑定动画专属的 VS 和 InputLayout
	g_pContext->VSSetShader(g_pSkinningShadowVS, nullptr, 0);
	g_pContext->PSSetShader(nullptr, nullptr, 0); // 阴影生成不需要 PS
	g_pContext->IASetInputLayout(g_pSkinningShadowInputLayout);

	// 矩阵常量缓冲区同样绑定在 b0
	g_pContext->VSSetConstantBuffers(0, 1, &g_pShadowConstantBuffer);
	g_pContext->RSSetState(g_pShadowRasterizer);
}

void Shader_Shadow_Finalize()
{
	if (g_pShadowVS) g_pShadowVS->Release();
	if (g_pShadowInputLayout) g_pShadowInputLayout->Release();
	if (g_pSkinningShadowVS) g_pSkinningShadowVS->Release();
	if (g_pSkinningShadowInputLayout) g_pSkinningShadowInputLayout->Release();
	if (g_pShadowConstantBuffer) g_pShadowConstantBuffer->Release();
	if (g_pShadowMapTexture) g_pShadowMapTexture->Release();
	if (g_pShadowDSV) g_pShadowDSV->Release();
	if (g_pShadowSRV) g_pShadowSRV->Release();
	if (g_pShadowRasterizer) g_pShadowRasterizer->Release();
}