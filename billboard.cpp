#include "billboard.h"
#include <DirectXMath.h>
#include "direct3d.h"
#include "shader_billboard.h"
#include "key_logger.h"
#include "mouse.h"
#include "texture.h"
#include "Player_Camera.h"

using namespace DirectX;
namespace {
	constexpr int NUM_VERTEX = 4; // 頂点数
	ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
}
struct Vertex3D
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4 color;
	XMFLOAT2 uv; // uv座標
};

void Billboard_Initialize()
{
	Shader_Billboard_Initialize();
	Vertex3D Vertex[]
	{
		{{-0.5f,  0.5f,0.0f},{ 1.0f,1.0f,1.0f,1.0f},  {0.0f, 0.0f}}, // 左上
		{{ 0.5f,  0.5f,0.0f},{ 1.0f,1.0f,1.0f,1.0f},  {1.0f, 0.0f}}, // 右上
		{{-0.5f, -0.5f,0.0f},{ 1.0f,1.0f,1.0f,1.0f},  {0.0f, 1.0f}}, // 左下
		{{ 0.5f, -0.5f,0.0f},{ 1.0f,1.0f,1.0f,1.0f},  {1.0f, 1.0f}}, // 右下
	};

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;// 書き込み不可に設定
	bd.ByteWidth = sizeof(Vertex3D) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = Vertex;
	Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &g_pVertexBuffer);
	
	return;
}

void Billboard_Finalize(void)
{
	SAFE_RELEASE(g_pVertexBuffer);
	Shader_Billboard_Finalize();
}

void Billboard_Draw(int texID,const DirectX::XMFLOAT3& position,const DirectX::XMFLOAT2& scale,const DirectX::XMFLOAT2& pivot)
{

	Shader_Billboard_SetUVParameter({ { 1.0f,1.0f}, { 0.0f,0.0f } });

	// シェーダーを描画パイプラインに設定
	Shader_Billboard_Begin();

	Shader_Billboard_SetColor({ 1.0f,1.0f,1.0f,1.0f });

	Texture_Set(texID);
	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	Direct3D_GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

	XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x , -pivot.y , 0.0f);

	XMFLOAT4X4 CameraMatrix = Player_Camera_GetViewMatrix();
	CameraMatrix._41 = CameraMatrix._42 = CameraMatrix._43 = 0.0f;


	//XMMATRIX iv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&CameraMatrix));
	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&CameraMatrix));
	XMMATRIX mtxs = XMMatrixScaling(scale.x, scale.y, 1.0f);
	XMMATRIX mtxt = XMMatrixTranslation(position.x + pivot.x ,position.y + pivot.y ,position.z);
	Shader_Billboard_SetWorldMatrix(pivot_offset* mtxs * iv * mtxt);

	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	// ポリゴン描画命令発行
	Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}

void Billboard_Draw(int texID, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4& tex_cut, const DirectX::XMFLOAT2& pivot)
{
	Shader_Billboard_Begin();
	
	float uv_x = tex_cut.x / Texture_GetWidth(texID);
	float uv_y = tex_cut.y / Texture_GetHeight(texID);
	float uv_w = tex_cut.z / Texture_GetWidth(texID);
	float uv_h = tex_cut.w / Texture_GetHeight(texID);

	// シェーダーを描画パイプラインに設定
	Shader_Billboard_SetUVParameter({ { uv_w,uv_h}, { uv_x,uv_y } });

	Shader_Billboard_SetColor({ 1.0f,1.0f,1.0f,1.0f });

	Texture_Set(texID);
	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	Direct3D_GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

	XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);

	XMFLOAT4X4 CameraMatrix = Player_Camera_GetViewMatrix();
	CameraMatrix._41 = CameraMatrix._42 = CameraMatrix._43 = 0.0f;


	//XMMATRIX iv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&CameraMatrix));
	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&CameraMatrix));
	XMMATRIX mtxs = XMMatrixScaling(scale.x, scale.y, 1.0f);
	XMMATRIX mtxt = XMMatrixTranslation(position.x + pivot.x, position.y + pivot.y, position.z);
	Shader_Billboard_SetWorldMatrix(pivot_offset * mtxs * iv * mtxt);

	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	// ポリゴン描画命令発行
	Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}
