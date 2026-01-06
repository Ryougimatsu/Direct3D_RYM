#include "collision.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <limits> 
#include <cmath>  
#include "debug_ostream.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"
#include <algorithm>
using namespace DirectX;



bool Collision_OverlapCircleCircle(const Circle& a, const Circle& b)
{
	float x1 = b.center.x - a.center.x;
	float y1 = b.center.y - a.center.y;

	return (a.radius + b.radius) * (a.radius + b.radius) > (x1 * x1 + y1 * y1);

	//XMVECTOR centerA = XMLoadFloat2(&a.center);
	//XMVECTOR centerB = XMLoadFloat2(&b.center);
	//XMVECTOR lsq = XMVector2LengthSq(centerB - centerA);

	//return (a.radius + b.radius) * (a.radius + b.radius) > XMVectorGetX(lsq);
}

bool Collision_OverlapCircleBox(const Box& a, const Box& b)
{
	float at = a.center.y - a.halfHeight;
	float ab = a.center.y + a.halfHeight;
	float al = a.center.x - a.halfWidth;
	float ar = a.center.x + a.halfWidth;
	float bt = b.center.y - b.halfHeight;
	float bb = b.center.y + b.halfHeight;
	float bl = b.center.x - b.halfWidth;
	float br = b.center.x + b.halfWidth;

	return al < br && ar > bl && at < bb && ab > bt;
}


static constexpr int NUM_VERTEX = 5000; // 頂点数の上限

namespace
{
	ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
	ID3D11Device* g_pDevice = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;
	int g_WhiteId = -1; // 白色のテクスチャID
}

struct Vertex
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4 color;
	XMFLOAT2 uv;
};

bool Collision_IsOverlapSphere(const Sphere& a, const Sphere& b)
{
	XMVECTOR centerA = XMLoadFloat3(&a.center);
	XMVECTOR centerB = XMLoadFloat3(&b.center);
	XMVECTOR lsq = XMVector3LengthSq(centerB - centerA);

	return (a.radius + b.radius) * (a.radius + b.radius) > XMVectorGetX(lsq);
}

bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point)
{
	XMVECTOR centerA = XMLoadFloat3(&a.center);
	XMVECTOR centerB = XMLoadFloat3(&point);
	XMVECTOR lsq = XMVector3LengthSq(centerB - centerA);

	return a.radius * a.radius > XMVectorGetX(lsq);
}

bool Collision_IsOverLapAABB(const AABB& a, const AABB& b)
{
	return a.min.x <= b.max.x
		&& a.max.x >= b.min.x
		&& a.min.y <= b.max.y
		&& a.max.y >= b.min.y
		&& a.min.z <= b.max.z
		&& a.max.z >= b.min.z;
}

bool Collision_IsOverlapSphereAABB(const Sphere& sphere, const AABB& aabb)
{
	// 1. 寻找 AABB 上距离球心最近的点 (Clamp 操作)
	// 将球心的 x, y, z 限制在 AABB 的 min 和 max 之间
	float closestX = std::max(aabb.min.x, std::min(sphere.center.x, aabb.max.x));
	float closestY = std::max(aabb.min.y, std::min(sphere.center.y, aabb.max.y));
	float closestZ = std::max(aabb.min.z, std::min(sphere.center.z, aabb.max.z));

	// 2. 计算最近点到球心的距离的平方
	float distanceX = sphere.center.x - closestX;
	float distanceY = sphere.center.y - closestY;
	float distanceZ = sphere.center.z - closestZ;

	float distanceSq = (distanceX * distanceX) + (distanceY * distanceY) + (distanceZ * distanceZ);

	// 3. 如果距离平方小于半径平方，说明碰撞了
	return distanceSq < (sphere.radius * sphere.radius);
}

Hit Collision_IsHitAABB(const AABB& a, const AABB& b)
{
	Hit hit{};
	hit.isHit = Collision_IsOverLapAABB(a, b);
	if (!hit.isHit) return hit;

	float xdepth = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
	float ydepth = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
	float zdepth = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

	DirectX::XMFLOAT3 aCenter = a.GetCenter();
	DirectX::XMFLOAT3 bCenter = b.GetCenter();
	DirectX::XMFLOAT3 normal = { 0,0,0 };

	if (xdepth <= ydepth && xdepth <= zdepth) {
		normal.x = (bCenter.x > aCenter.x) ? 1.0f : -1.0f;
	}
	else if (ydepth <= xdepth && ydepth <= zdepth) {
		normal.y = (bCenter.y > aCenter.y) ? 1.0f : -1.0f;
	}
	else {
		normal.z = (bCenter.z > aCenter.z) ? 1.0f : -1.0f;
	}
	hit.normal = normal;
	return hit;
}


bool Collision_IntersectRayAABB(const Ray& ray, const AABB& box, float& outDist)
{
	float tMin = 0.0f;
	float tMax = std::numeric_limits<float>::max();

	// 分别获取射线起点(O)、方向(D) 和 AABB的范围(minB, maxB)
	float O[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
	float D[3] = { ray.direction.x, ray.direction.y, ray.direction.z };
	float minB[3] = { box.min.x, box.min.y, box.min.z };
	float maxB[3] = { box.max.x, box.max.y, box.max.z };

	// 遍历 X, Y, Z 三个轴进行切片测试
	for (int i = 0; i < 3; ++i)
	{
		// 如果射线在这个轴上近似平行 (方向分量接近0)
		if (std::abs(D[i]) < 1e-6f)
		{
			// 如果起点不在盒子在这个轴的范围内，则射线完全错过盒子
			if (O[i] < minB[i] || O[i] > maxB[i])
				return false;
		}
		else
		{
			// 计算射线穿入平面(t1)和穿出平面(t2)的距离
			float t1 = (minB[i] - O[i]) / D[i];
			float t2 = (maxB[i] - O[i]) / D[i];

			// 确保 t1 是近平面，t2 是远平面
			if (t1 > t2) std::swap(t1, t2);

			// 更新整体的 tMin (最晚进入时间) 和 tMax (最早离开时间)
			tMin = std::max(tMin, t1);
			tMax = std::min(tMax, t2);

			// 如果错过了 (离开比进入早) 或者 盒子在射线背面 (tMax < 0)
			if (tMin > tMax || tMax < 0.0f) return false;
		}
	}

	// 检测通过，返回最近的交点距离
	outDist = tMin;
	return true;
}

void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{

	g_WhiteId = Texture_LoadFromFile(L"resource/texture/white.png"); // 白色のテクスチャを読み込む
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

}

void Collision_DebugFinalize()
{
	SAFE_RELEASE(g_pVertexBuffer);
}

void Collision_DebugDraw(const Circle& circle, const DirectX::XMFLOAT4& color)
{
	int vertexNum = static_cast<int>(circle.radius * XM_2PI + 1);

	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	const float rad = 2.0f * DirectX::XM_PI / vertexNum; // 1頂点あたりの角度

	for (int i = 0; i < vertexNum; ++i) {
		float angle = rad * i; // 現在の角度
		v[i].position = {
			circle.center.x + circle.radius * cosf(angle),
			circle.center.y + circle.radius * sinf(angle),
			0.0f
		};
		v[i].color = color;
		v[i].uv = { 0.0f,0.0f };
	}

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	Shader_SetWorldMatrix(XMMatrixIdentity());


	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	Texture_Set(g_WhiteId); // 白色のテクスチャを設定;

	g_pContext->Draw(vertexNum, 0);
}

void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4& color)
{

	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	v[0].position = { box.center.x - box.halfWidth,box.center.y - box.halfHeight,0.0f };
	v[1].position = { box.center.x + box.halfWidth,box.center.y - box.halfHeight,0.0f };
	v[2].position = { box.center.x + box.halfWidth,box.center.y + box.halfHeight,0.0f };
	v[3].position = { box.center.x - box.halfWidth,box.center.y + box.halfHeight,0.0f };

	for (int i = 0; i < 4; ++i) {

		v[i].color = color;
		v[i].uv = { 0.0f,0.0f };
	}

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	Shader_SetWorldMatrix(XMMatrixIdentity());


	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	Texture_Set(g_WhiteId); // 白色のテクスチャを設定;

	g_pContext->Draw(4, 0);
}

void Collision_DebugDraw(const AABB& aabb, const DirectX::XMFLOAT4& color)
{
	// 1. 获取AABB的8个顶点（角）
	DirectX::XMFLOAT3 corners[8] = {
		{ aabb.min.x, aabb.min.y, aabb.min.z }, // 0: Bottom-Back-Left
		{ aabb.max.x, aabb.min.y, aabb.min.z }, // 1: Bottom-Back-Right
		{ aabb.max.x, aabb.max.y, aabb.min.z }, // 2: Top-Back-Right
		{ aabb.min.x, aabb.max.y, aabb.min.z }, // 3: Top-Back-Left
		{ aabb.min.x, aabb.min.y, aabb.max.z }, // 4: Bottom-Front-Left
		{ aabb.max.x, aabb.min.y, aabb.max.z }, // 5: Bottom-Front-Right
		{ aabb.max.x, aabb.max.y, aabb.max.z }, // 6: Top-Front-Right
		{ aabb.min.x, aabb.max.y, aabb.max.z }  // 7: Top-Front-Left
	};

	// 2. 定义12条线 (需要24个顶点)
	Vertex v[24];

	auto set_line = [&](int v_index, int corner_a, int corner_b) {
		v[v_index].position = corners[corner_a];
		v[v_index].color = color;
		v[v_index].uv = { 0.0f, 0.0f };

		v[v_index + 1].position = corners[corner_b];
		v[v_index + 1].color = color;
		v[v_index + 1].uv = { 0.0f, 0.0f };
		};

	// 绘制底面 (4条线)
	set_line(0, 0, 1);
	set_line(2, 1, 5);
	set_line(4, 5, 4);
	set_line(6, 4, 0);

	// 绘制顶面 (4条线)
	set_line(8, 3, 2);
	set_line(10, 2, 6);
	set_line(12, 6, 7);
	set_line(14, 7, 3);

	// 绘制垂直的边 (4条线)
	set_line(16, 0, 3);
	set_line(18, 1, 2);
	set_line(20, 5, 6);
	set_line(22, 4, 7);



	Shader_Begin();

	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);


	memcpy(msr.pData, v, sizeof(Vertex) * 24);

	g_pContext->Unmap(g_pVertexBuffer, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	Shader_SetWorldMatrix(DirectX::XMMatrixIdentity());

	
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	Texture_Set(g_WhiteId);

	// *** 注意：绘制24个顶点 ***
	g_pContext->Draw(24, 0);
}