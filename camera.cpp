#include "camera.h"
#include "direct3d.h"

using namespace DirectX;

namespace
{
	ID3D11Buffer* g_pViewBuffer = nullptr;
	ID3D11Buffer* g_pProjectionBuffer = nullptr;
}

void Camera_Initialize()
{
	if (g_pViewBuffer && g_pProjectionBuffer)
		return;

	Camera_Finalize();

	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = sizeof(XMFLOAT4X4);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Device* device = Direct3D_GetDevice();
	if (!device)
		return;

	if (FAILED(device->CreateBuffer(&bufferDesc, nullptr, &g_pViewBuffer)) ||
		FAILED(device->CreateBuffer(&bufferDesc, nullptr, &g_pProjectionBuffer)))
	{
		Camera_Finalize();
	}
}

void Camera_Finalize()
{
	SAFE_RELEASE(g_pViewBuffer);
	SAFE_RELEASE(g_pProjectionBuffer);
}

void Camera_SetMatrixToShader(const XMMATRIX& view, const XMMATRIX& proj)
{
	if (!g_pViewBuffer || !g_pProjectionBuffer)
		return;

	ID3D11DeviceContext* context = Direct3D_GetDeviceContext();
	if (!context)
		return;

	XMFLOAT4X4 transpose;
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(view));
	context->UpdateSubresource(g_pViewBuffer, 0, nullptr, &transpose, 0, 0);

	XMStoreFloat4x4(&transpose, XMMatrixTranspose(proj));
	context->UpdateSubresource(g_pProjectionBuffer, 0, nullptr, &transpose, 0, 0);

	context->VSSetConstantBuffers(1, 1, &g_pViewBuffer);
	context->VSSetConstantBuffers(2, 1, &g_pProjectionBuffer);
}
