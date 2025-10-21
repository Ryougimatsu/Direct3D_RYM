#include "Light.h"
using namespace DirectX;

namespace {
	ID3D11Buffer* g_pVSConstantBuffer3 = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;
	ID3D11Device* g_pDevice = nullptr;
	ID3D11Buffer* g_pVSConstantBuffer4 = nullptr;
}

class DirectionalLight
{
public:
	XMFLOAT4 Directional;
	XMFLOAT4 Color;
};

void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.ByteWidth = sizeof(XMFLOAT4); 


	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer3);

	buffer_desc.ByteWidth = sizeof(DirectionalLight);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer4);
}

void Light_Finalize(void)
{
	if (g_pVSConstantBuffer4) {
		g_pVSConstantBuffer4->Release();
		g_pVSConstantBuffer4 = nullptr;
	}
	if (g_pVSConstantBuffer3) {
		g_pVSConstantBuffer3->Release();
		g_pVSConstantBuffer3 = nullptr;
	}
}

void Light_SetAmbient(const DirectX :: XMFLOAT3& color)
{
	g_pContext->UpdateSubresource(g_pVSConstantBuffer3, 0, nullptr, &color, 0, 0);
	g_pContext->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer3);
}

void Light_SetDirectionalWorld(const DirectX::XMFLOAT4& world_dir, const DirectX::XMFLOAT4& color)
{
	DirectionalLight light{
		world_dir,
		color
	};
	g_pContext->UpdateSubresource(g_pVSConstantBuffer4, 0, nullptr, &light, 0, 0);
	g_pContext->VSSetConstantBuffers(4, 1, &g_pVSConstantBuffer4);

}
