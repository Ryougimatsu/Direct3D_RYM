#include "billboard.h"
#include <DirectXMath.h>
#include "direct3d.h"
#include "shader_billboard.h"
#include "key_logger.h"
#include "mouse.h"
#include "texture.h"
#include "Player_Camera.h"
#include "camera.h"

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

	XMFLOAT4X4 CameraMatrix = Player_Camera_GetMatrix();
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

void Billboard_Draw(int texID, DirectX::XMFLOAT3 pos, float width, float height, float u, float v, float uw, float vh)
{
	// 1. 设置 Shader
	Shader_Billboard_Begin();
	Shader_Billboard_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// 2. 直接设置 UV (不除以宽高)
	// 注意：根据你的 Shader_Billboard_SetUVParameter 定义，参数顺序可能是 {{uw, vh}, {u, v}}
	// 请对照你的 Shader_Billboard.h 确认结构体顺序
	Shader_Billboard_SetUVParameter({ { uw, vh }, { u, v } });

	// 3. 设置纹理和顶点
	Texture_Set(texID);
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	Direct3D_GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

	// 4. 计算矩阵 (Billboard 核心)
	// 使用 Player_Camera 或者通用的 Camera_GetViewMatrix
	XMFLOAT4X4 CameraMatrix = Player_Camera_GetViewMatrix();
	CameraMatrix._41 = CameraMatrix._42 = CameraMatrix._43 = 0.0f;

	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&CameraMatrix));

	// 使用传入的 width 和 height 作为缩放
	XMMATRIX mtxs = XMMatrixScaling(width, height, 1.0f);

	// 使用传入的 pos 作为位置 (Pivot 默认为 0)
	XMMATRIX mtxt = XMMatrixTranslation(pos.x, pos.y, pos.z);

	// 组合矩阵
	Shader_Billboard_SetWorldMatrix(mtxs * iv * mtxt);

	// 5. 绘制
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}

ID3D11Buffer* Billboard_GetVertexBuffer() {
	return g_pVertexBuffer;
}

void Laser_Billboard_Draw(int texid, XMVECTOR start, XMVECTOR end, float width)
{
	// 1. 计算线段信息
	XMVECTOR dir = end - start;
	float length = XMVectorGetX(XMVector3Length(dir));
	if (length < 0.0001f) return;

	XMVECTOR midPos = start + dir * 0.5f;

	// 2. 构造朝向矩阵（轴向 billboard）
	XMVECTOR up = XMVector3Normalize(dir);  // 激光方向 -> Y

	XMVECTOR camPos = XMLoadFloat3(&Player_Camera_GetPosition());
	XMVECTOR lookAt = XMVector3Normalize(camPos - midPos);

	XMVECTOR right = XMVector3Normalize(XMVector3Cross(lookAt, up));
	XMVECTOR forward = XMVector3Cross(right, up);
	XMMATRIX mtxR;
	mtxR.r[0] = right;
	mtxR.r[1] = up;
	mtxR.r[2] = forward;
	mtxR.r[3] = XMVectorSet(0, 0, 0, 1);

	XMMATRIX mtxS = XMMatrixScaling(width, length, 1.0f);
	XMMATRIX mtxT = XMMatrixTranslationFromVector(midPos);

	Shader_Billboard_Begin();


	Shader_Billboard_SetUVParameter({ { 1.0f, 1.0f }, { 0.0f, 0.0f } });


	Shader_Billboard_SetColor({ 1.0f, 0.0f, 0.0f, 0.8f });

	Shader_Billboard_SetWorldMatrix(mtxS * mtxR * mtxT);

	Texture_Set(texid);

	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	ID3D11DeviceContext* context = Direct3D_GetDeviceContext();
	ID3D11Buffer* pVB = Billboard_GetVertexBuffer();
	context->IASetVertexBuffers(0, 1, &pVB, &stride, &offset);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	context->Draw(4, 0);
}

void Billboard_Draw(int texID, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot, const DirectX::XMFLOAT4& color)
{
	// 1. 设置 Shader 和 混合状态
	Shader_Billboard_Begin();

	// 【修正 1】使用参数传入的颜色 (color)，而不是写死的白色
	// 这样粒子才能变色或淡出
	Shader_Billboard_SetColor(color);

	// 【修正 2】设置默认 UV (显示整张图片)
	// 粒子通常使用整张纹理，不需要切图。参数顺序参考原代码：{{宽, 高}, {u, v}}
	Shader_Billboard_SetUVParameter({ { 1.0f, 1.0f }, { 0.0f, 0.0f } });

	// 3. 设置纹理和顶点缓冲
	Texture_Set(texID);
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	Direct3D_GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

	// 4. 计算矩阵 (Billboard 核心)
	XMFLOAT4X4 CameraMatrix = Player_Camera_GetViewMatrix();
	// 消除摄像机位移，只保留旋转，这样公告板才会“看”向摄像机
	CameraMatrix._41 = CameraMatrix._42 = CameraMatrix._43 = 0.0f;

	// 转置矩阵 (DirectXMath 中处理 View 矩阵逆变换的一种常用方式)
	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&CameraMatrix));

	// 【修正 3】Pivot 偏移 (让缩放和旋转围绕中心点)
	XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);

	// 【修正 4】使用参数 scale.x 和 scale.y，而不是未定义的 width/height
	XMMATRIX mtxs = XMMatrixScaling(scale.x, scale.y, 1.0f);

	// 【修正 5】使用参数 position，而不是未定义的 pos
	// 加上 pivot 恢复偏移
	XMMATRIX mtxt = XMMatrixTranslation(position.x + pivot.x, position.y + pivot.y, position.z);

	// 组合矩阵: 偏移 -> 缩放 -> 旋转(Billboard) -> 移动
	Shader_Billboard_SetWorldMatrix(pivot_offset * mtxs * iv * mtxt);

	// 5. 绘制
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}

void MuzzleFlash_Cross_Draw(int texid, DirectX::XMVECTOR muzzlePos, DirectX::XMVECTOR gunForwardDir, float width, float height, float alpha)
{
	// 1. 设置 Shader
	Shader_Billboard_Begin();
	Shader_Billboard_SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	Shader_Billboard_SetUVParameter({ { 1.0f, 1.0f }, { 0.0f, 0.0f } });
	Texture_Set(texid);

	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	ID3D11DeviceContext* context = Direct3D_GetDeviceContext();
	context->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

	// 关闭背面剔除 (确保十字面的两面都能看见) 引擎底层虽然设了 NONE，这里保险起见确认下
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// 2. 构造 3D 局部坐标系
	using namespace DirectX;
	XMVECTOR forward = XMVector3Normalize(gunForwardDir);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	if (fabsf(XMVectorGetY(forward)) > 0.99f) {
		up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	}
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(forward, up));
	up = XMVector3Normalize(XMVector3Cross(right, forward));

	// 【关键修复 1：取消巨大的中心偏移】
	// 让图片的中心尽量靠近枪管，只往前推 0.2f 防止嵌进枪管里。
	// 如果你发现特效太靠后了，可以适当加大这个 0.2f
	XMVECTOR offsetPos = muzzlePos + forward * 0.2f;
	XMMATRIX mtxT = XMMatrixTranslationFromVector(offsetPos);
	XMMATRIX mtxS = XMMatrixScaling(width, height, 1.0f);

	// 【关键修复 2：矩阵方向映射】
	// 默认情况：假设你的贴图里，火焰是【朝上(Y轴)】喷射的
	XMMATRIX mtx1;
	mtx1.r[0] = right;
	mtx1.r[1] = forward; // 图片的Y轴映射到枪管前方
	mtx1.r[2] = up;
	mtx1.r[3] = XMVectorSet(0, 0, 0, 1);

	// === ⚠️ 调试提示 ⚠️ ===
	// 如果你运行后发现火焰是“横向”长在枪管上的，说明你的贴图是【朝右(X轴)】喷射的！
	// 请注释掉上面两行，解开下面两行的注释：
	// mtx1.r[0] = forward; // 图片的X轴映射到枪管前方
	// mtx1.r[1] = right;   
	// ======================

	// 构造第二个交叉面 (绕着枪管自转 90 度)
	XMMATRIX rot90 = XMMatrixRotationAxis(forward, XM_PIDIV2);
	XMMATRIX mtx2;
	mtx2.r[0] = XMVector3TransformNormal(mtx1.r[0], rot90);
	mtx2.r[1] = XMVector3TransformNormal(mtx1.r[1], rot90);
	mtx2.r[2] = XMVector3TransformNormal(mtx1.r[2], rot90);
	mtx2.r[3] = XMVectorSet(0, 0, 0, 1);

	// 3. 绘制两个相交的面
	Shader_Billboard_SetWorldMatrix(mtxS * mtx1 * mtxT);
	context->Draw(4, 0);

	Shader_Billboard_SetWorldMatrix(mtxS * mtx2 * mtxT);
	context->Draw(4, 0);
}

void MuzzleFlash_Axial_Draw(int texid, DirectX::XMVECTOR muzzlePos, DirectX::XMVECTOR gunForwardDir, float width, float height, float alpha, const DirectX::XMMATRIX& viewMatrix)
{
	Shader_Billboard_Begin();
	Shader_Billboard_SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	Shader_Billboard_SetUVParameter({ { 1.0f, 1.0f }, { 0.0f, 0.0f } });
	Texture_Set(texid);

	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	ID3D11DeviceContext* context = Direct3D_GetDeviceContext();
	context->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	using namespace DirectX;

	// 1. 获取图片向前的 Y 轴对齐枪口方向
	XMVECTOR up = XMVector3Normalize(gunForwardDir);

	// 2. 【核心黑科技】：通过逆矩阵直接从渲染管线中抽出真实的摄像机坐标
	XMVECTOR det;
	XMMATRIX invView = XMMatrixInverse(&det, viewMatrix);
	XMVECTOR camPos = invView.r[3]; // 第 4 行就是摄像机的世界坐标！

	// 3. 计算指向摄像机的向量
	XMVECTOR lookAt = XMVector3Normalize(camPos - muzzlePos);

	if (fabsf(XMVectorGetX(XMVector3Dot(up, lookAt))) > 0.999f) {
		lookAt = XMVectorAdd(lookAt, XMVectorSet(0.01f, 0.01f, 0.0f, 0.0f));
		lookAt = XMVector3Normalize(lookAt);
	}

	// 【最终修复】：用 (枪口方向) 叉乘 (看向摄像机的方向) 来获得 X 轴
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, lookAt));

	// 然后用 (X 轴) 叉乘 (枪口方向) 来获得 Z 轴法线。
	// 这样不仅矩阵不会翻转，而且法线会完美指向顶部的摄像机！
	XMVECTOR forward = XMVector3Cross(right, up);

	XMMATRIX mtxR;
	mtxR.r[0] = right;
	mtxR.r[1] = up;      // Y轴 = 枪口方向
	mtxR.r[2] = forward; // Z轴(法线) = 朝向摄像机
	mtxR.r[3] = XMVectorSet(0, 0, 0, 1);

	// 4. 【关键修复】：取消巨大的中心偏移，只往前推 0.2f
	XMVECTOR offsetPos = muzzlePos + up * 0.2f;
	XMMATRIX mtxT = XMMatrixTranslationFromVector(offsetPos);
	XMMATRIX mtxS = XMMatrixScaling(width, height, 1.0f);

	Shader_Billboard_SetWorldMatrix(mtxS * mtxR * mtxT);
	context->Draw(4, 0);
}