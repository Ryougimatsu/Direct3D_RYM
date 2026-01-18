#pragma once

// ----------------------------------------------------------------
// Includes
// ----------------------------------------------------------------
#include <DirectXMath.h>
#include <vector>
#include <list>

// ----------------------------------------------------------------
// Configuration Constants (Global)
// ----------------------------------------------------------------
// 定义一个简单的网格大小 (例如 20x20 米)
const int   GRID_WIDTH = 20;
const int   GRID_HEIGHT = 20;
const float GRID_SIZE = 1.0f;        // 每个格子 1米 x 1米

// 地图原点偏移 (让 (0,0) 在地图中心)
const float MAP_OFFSET_X = -10.0f;
const float MAP_OFFSET_Z = -10.0f;

// ----------------------------------------------------------------
// Data Structures
// ----------------------------------------------------------------
struct Node {
	int x, y;           // 网格坐标
	bool isObstacle;    // 是墙吗？
	bool closed;        // 是否在关闭列表中
	float gCost;        // 起点到这里的代价
	float hCost;        // 这里到终点的预估代价
	Node* parent;       // 父节点 (用于回溯路径)

	float FCost() const { return gCost + hCost; }
};

// ----------------------------------------------------------------
// Class Definition
// ----------------------------------------------------------------
class Pathfinder {
public:
	// ==========================================
	// 1. 生命周期 (Lifecycle)
	// ==========================================
	static void Initialize();
	static void Finalize(); // 如果有动态分配需清理

	// ==========================================
	// 2. 地图配置 (Map Configuration)
	// ==========================================
	// 设置障碍物 (传入世界坐标)
	static void SetObstacle(float worldX, float worldZ, bool isBlocked);

	// ==========================================
	// 3. 核心寻路 (Pathfinding Operations)
	// ==========================================
	// 输入起点和终点(世界坐标)，返回路径点列表
	static std::vector<DirectX::XMFLOAT3> FindPath(DirectX::XMFLOAT3 startPos, DirectX::XMFLOAT3 targetPos);

	// 视线检查 (Raycast)
	static bool RaycastHit(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end);

private:
	// ==========================================
	// 内部数据 (Internal Data)
	// ==========================================
	static Node m_Grid[GRID_WIDTH][GRID_HEIGHT]; // 静态网格数据

	// ==========================================
	// 辅助函数 (Helpers)
	// ==========================================
	// 世界坐标 <-> 网格坐标转换
	static void WorldToGrid(float wx, float wz, int& gx, int& gy);
	static DirectX::XMFLOAT3 GridToWorld(int gx, int gy);
};