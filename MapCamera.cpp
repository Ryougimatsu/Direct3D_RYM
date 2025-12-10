#include "MapCamera.h"
using namespace DirectX;
namespace
{
	XMFLOAT3 g_Position{};
	XMFLOAT3 g_Front{};
}
void MapCam_Initialize()
{

}
void MapCam_Finalize()
{

}

void MapCam_SetFront(const DirectX::XMFLOAT3& front)
{
	g_Front = front;
}
void MapCam_SetPosition(const DirectX::XMFLOAT3& position)
{
	g_Position = position;
}

const DirectX::XMFLOAT4X4& MapCam_GetViewMatrix()
{
	XMFLOAT4X4 mtxView;
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&g_Position),
		XMLoadFloat3(&g_Front),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	);
	XMStoreFloat4x4(&mtxView, view);
	return mtxView;
}
const DirectX::XMFLOAT4X4& MapCam_GetPerspectiveMatrix()
{
	XMFLOAT4X4 mtxPerspective;
	XMMATRIX perspective = XMMatrixOrthographicOffCenterLH(-10.0f, 10.0f, 10.0f, -10.0f, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mtxPerspective, perspective);
	return mtxPerspective;
}