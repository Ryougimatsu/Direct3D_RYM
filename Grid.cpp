#include "Grid.h"
#include "direct3d.h"
#include "shader_3d.h"
#include <DirectXMath.h>

using namespace DirectX;

// 定义网格常量
static constexpr int GRID_H_COUNT = 10; // 水平方向的格子数量
static constexpr int GRID_V_COUNT = 10; // 垂直方向的格子数量
static constexpr int GRID_H_LINE_COUNT = GRID_H_COUNT + 1; // 水平方向的线条数量
static constexpr int GRID_V_LINE_COUNT = GRID_V_COUNT + 1; // 垂直方向的线条数量
static constexpr int NUM_VERTEX = GRID_H_LINE_COUNT * 2 + GRID_V_LINE_COUNT * 2; // 总顶点数（每条线2个顶点）

// D3D11 资源
static ID3D11Buffer* g_pVertexBuffer = nullptr; // 顶点缓冲区

// 由外部传入的D3D11接口，无需在此文件中Release
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 3D顶点结构体
// 注意：此结构体必须与着色器的输入布局(Input Layout)完全匹配
struct Vertex3D
{
	XMFLOAT3 position; // 顶点坐标 (POSITION)
	XMFLOAT4 color;    // 顶点颜色 (COLOR)
	XMFLOAT2 uv;       // 纹理坐标 (TEXCOORD)
};

// 存储所有网格线顶点的数组
static Vertex3D g_GridVertices[NUM_VERTEX]{};

void Grid_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 保存由外部传入的设备和设备上下文指针
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 生成水平网格线的顶点数据
	for (int i = 0; i < GRID_H_LINE_COUNT; ++i) {
		float x = -5.0f + (10.0f / GRID_H_COUNT) * i;
		// 虽然绘制线条不使用UV，但为满足顶点结构体定义，仍需初始化
		g_GridVertices[i * 2 + 0] = { {x, 0.0f, -5.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} }; // 线的起点, 颜色为白色
		g_GridVertices[i * 2 + 1] = { {x, 0.0f,  5.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} }; // 线的终点, 颜色为白色
	}

	// 生成垂直网格线的顶点数据
	int offset = GRID_H_LINE_COUNT * 2;
	for (int i = 0; i < GRID_V_LINE_COUNT; ++i) {
		float z = -5.0f + (10.0f / GRID_V_COUNT) * i;
		g_GridVertices[offset + i * 2 + 0] = { {-5.0f, 0.0f, z}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} }; // 线的起点
		g_GridVertices[offset + i * 2 + 1] = { { 5.0f, 0.0f, z}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} }; // 线的终点
	}

	// --- 创建顶点缓冲区 ---
	// 1. 填充缓冲区描述
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex3D) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	// 2. 填充子资源数据
	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_GridVertices;

	// 3. 创建缓冲区
	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);
}

void Grid_Finalize(void)
{
	// 释放此文件中创建的D3D资源
	if (g_pVertexBuffer) {
		g_pVertexBuffer->Release();
		g_pVertexBuffer = nullptr;
	}
}

void Grid_Draw(void)
{
	// 绑定着色器、输入布局和常量缓冲区
	Shader_3D_Begin();

	// 将顶点缓冲区设置到渲染管线
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// 设置世界矩阵为单位矩阵（表示模型没有额外的移动、旋转或缩放）
	XMMATRIX mtxWorld = XMMatrixIdentity();
	Shader_3D_SetWorldMatrix(mtxWorld);

	// 设置图元拓扑为线段列表（每2个顶点构成一条独立的线）
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 发出绘制指令，绘制所有顶点
	g_pContext->Draw(NUM_VERTEX, 0);
}