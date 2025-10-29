#include "direct3d.h"
#include "Light.h"
using namespace DirectX;

//並行光源（拡散反射）
struct DirectionalLight
{
	XMFLOAT4 Directional;
	XMFLOAT4 Color;
};

//鏡面反射光
struct SpecularLight
{
	XMFLOAT3 CameraPosition;
	float SpecularPower;
	XMFLOAT4 SpecularColor;
};

//点光源
struct PointLight
{
	DirectX::XMFLOAT3 LightPosition;
	float range;
	DirectX::XMFLOAT4 Color;
	//float SpecularStrength;
	//XMFLOAT3 SpecularColor;
};

//点光源リスト
struct PointLightList
{
	PointLight pointLights[4];
	int numPointLights;
	DirectX::XMFLOAT3 padding;
};

namespace {
	ID3D11Buffer* g_pPSConstantBuffer1 = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;
	ID3D11Device* g_pDevice = nullptr;
	ID3D11Buffer* g_pPSConstantBuffer2 = nullptr;
	ID3D11Buffer* g_pPSConstantBuffer3 = nullptr;
	ID3D11Buffer* g_pPSConstantBuffer4 = nullptr;

	PointLightList g_pointLights{};
}


void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	buffer_desc.ByteWidth = sizeof(XMFLOAT4);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer1);

	buffer_desc.ByteWidth = sizeof(DirectionalLight);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer2);

	buffer_desc.ByteWidth = sizeof(SpecularLight);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer3);

	buffer_desc.ByteWidth = sizeof(PointLightList);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer4);

}

void Light_Finalize(void)
{
	SAFE_RELEASE(g_pPSConstantBuffer4)
		SAFE_RELEASE(g_pPSConstantBuffer3)
		SAFE_RELEASE(g_pPSConstantBuffer2)
		SAFE_RELEASE(g_pPSConstantBuffer1)
}

void Light_SetAmbient(const DirectX::XMFLOAT3& color)
{
	g_pContext->UpdateSubresource(g_pPSConstantBuffer1, 0, nullptr, &color, 0, 0);
	g_pContext->PSSetConstantBuffers(1, 1, &g_pPSConstantBuffer1);
}

void Light_SetDirectionalWorld(
	const DirectX::XMFLOAT4& direction,
	const DirectX::XMFLOAT4& color)
{
	DirectionalLight light{ direction,color, };

	g_pContext->UpdateSubresource(g_pPSConstantBuffer2, 0, nullptr, &light, 0, 0);
	g_pContext->PSSetConstantBuffers(2, 1, &g_pPSConstantBuffer2);
}

void Light_SetSpecularWorld(
	const DirectX::XMFLOAT3& camera_position,
	const float& specular_power,
	const DirectX::XMFLOAT4& specular_color)
{
	SpecularLight light{
	 camera_position,
	 specular_power,
	 specular_color,
	};

	g_pContext->UpdateSubresource(g_pPSConstantBuffer3, 0, nullptr, &light, 0, 0);
	g_pContext->PSSetConstantBuffers(3, 1, &g_pPSConstantBuffer3);
}


void Light_SetPointLightCount(int count)
{
	g_pointLights.numPointLights = count;

	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_pointLights, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);
}

void Light_SetPointLightWorldByCount(const int index, const XMFLOAT3& position, float range, const XMFLOAT3& color)
{
	if (index >= g_pointLights.numPointLights)
	{
		return;
	}

	g_pointLights.pointLights[index].LightPosition = position;
	g_pointLights.pointLights[index].range = range;
	g_pointLights.pointLights[index].Color = { color.x,color.y,color.z,1.0f };

	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_pointLights, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);
}




