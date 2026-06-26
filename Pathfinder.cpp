#include "Pathfinder.h"
#include <cmath>
#include <algorithm>
#include <vector>

using namespace DirectX;

// 静态成员定义
Node Pathfinder::m_Grid[GRID_WIDTH][GRID_HEIGHT];

// ======================================================================================
// 1. 生命周期 (Lifecycle)
// ======================================================================================

void Pathfinder::Initialize() {
	// 初始化网格：默认全部为空地
	for (int x = 0; x < GRID_WIDTH; x++) {
		for (int y = 0; y < GRID_HEIGHT; y++) {
			m_Grid[x][y].x = x;
			m_Grid[x][y].y = y;
			m_Grid[x][y].isObstacle = false;
		}
	}
}

void Pathfinder::Finalize() {
	// 静态数组不需要特殊清理，如果改为动态内存分配则需在此 delete
}

// ======================================================================================
// 2. 地图配置与坐标转换 (Map Config & Conversion)
// ======================================================================================

void Pathfinder::SetObstacle(float worldX, float worldZ, bool isBlocked) {
	int gx, gy;
	WorldToGrid(worldX, worldZ, gx, gy);
	if (gx >= 0 && gx < GRID_WIDTH && gy >= 0 && gy < GRID_HEIGHT) {
		m_Grid[gx][gy].isObstacle = isBlocked;
	}
}

void Pathfinder::WorldToGrid(float wx, float wz, int& gx, int& gy) {
	gx = (int)((wx - MAP_OFFSET_X) / GRID_SIZE);
	gy = (int)((wz - MAP_OFFSET_Z) / GRID_SIZE);
}

XMFLOAT3 Pathfinder::GridToWorld(int gx, int gy) {
	return {
		gx * GRID_SIZE + MAP_OFFSET_X + GRID_SIZE * 0.5f, // +0.5 使得坐标位于格子中心
		0.0f,
		gy * GRID_SIZE + MAP_OFFSET_Z + GRID_SIZE * 0.5f
	};
}

// ======================================================================================
// 3. 核心寻路算法 (A* Pathfinding)
// ======================================================================================

std::vector<XMFLOAT3> Pathfinder::FindPath(XMFLOAT3 startPos, XMFLOAT3 targetPos) {
	std::vector<XMFLOAT3> path;

	// 1. 转换坐标：将世界坐标转换为网格索引
	int startX, startY, targetX, targetY;
	WorldToGrid(startPos.x, startPos.z, startX, startY);
	WorldToGrid(targetPos.x, targetPos.z, targetX, targetY);

	// 2. 边界检查
	if (startX < 0 || startX >= GRID_WIDTH || startY < 0 || startY >= GRID_HEIGHT ||
		targetX < 0 || targetX >= GRID_WIDTH || targetY < 0 || targetY >= GRID_HEIGHT) {
		return path; // 超出地图范围，无法寻路
	}

	// 3. 目标点有效性检查：如果终点是墙，或起点等于终点，直接返回
	if (m_Grid[targetX][targetY].isObstacle || (startX == targetX && startY == targetY)) {
		return path;
	}

	// 4. 重置网格数据 (为本次寻路做准备)
	for (int x = 0; x < GRID_WIDTH; x++) {
		for (int y = 0; y < GRID_HEIGHT; y++) {
			m_Grid[x][y].gCost = 99999.0f;
			m_Grid[x][y].hCost = 0.0f;
			m_Grid[x][y].parent = nullptr;
			m_Grid[x][y].closed = false;
		}
	}

	// 5. 初始化 OpenList
	// (注：简单的 vector 实现，高性能需求建议用 priority_queue)
	std::vector<Node*> openList;

	Node* startNode = &m_Grid[startX][startY];
	startNode->gCost = 0;
	// H值计算 (曼哈顿距离：绝对值之和，计算速度最快)
	startNode->hCost = (float)(abs(targetX - startX) + abs(targetY - startY));
	openList.push_back(startNode);

	// 6. 开始 A* 循环
	while (!openList.empty()) {
		// --- 6.1 找出 F 值最小的节点 ---
		Node* current = openList[0];
		size_t currentIndex = 0;
		for (size_t i = 1; i < openList.size(); i++) {
			if (openList[i]->FCost() < current->FCost() ||
				(openList[i]->FCost() == current->FCost() && openList[i]->hCost < current->hCost)) {
				current = openList[i];
				currentIndex = i;
			}
		}

		// 移出 OpenList，加入 ClosedList
		openList.erase(openList.begin() + currentIndex);
		current->closed = true;

		// --- 6.2 找到终点了吗？ ---
		if (current->x == targetX && current->y == targetY) {
			// 回溯路径
			Node* curr = current;
			while (curr != nullptr) {
				path.push_back(GridToWorld(curr->x, curr->y));
				curr = curr->parent;
			}
			// 路径是反的 (终点->起点)，需要翻转
			std::reverse(path.begin(), path.end());
			return path;
		}

		// --- 6.3 遍历邻居 (上下左右 4 方向) ---
		// 如果想支持斜向移动，这里需要改为 8 个方向
		int dx[4] = { 0, 0, 1, -1 };
		int dy[4] = { 1, -1, 0, 0 };

		for (int i = 0; i < 4; i++) {
			int checkX = current->x + dx[i];
			int checkY = current->y + dy[i];

			// 越界检查
			if (checkX < 0 || checkX >= GRID_WIDTH || checkY < 0 || checkY >= GRID_HEIGHT) continue;

			Node* neighbor = &m_Grid[checkX][checkY];

			// 障碍物或已关闭节点跳过
			if (neighbor->isObstacle || neighbor->closed) continue;

			// 计算新的 G 值 (相邻格子距离为 1)
			float newMovementCostToNeighbor = current->gCost + 1.0f;

			// 检查是否已在 OpenList 中
			bool inOpenList = false;
			for (auto* n : openList) if (n == neighbor) inOpenList = true;

			// 如果发现了更短的路径，或者该邻居不在开放列表中
			if (newMovementCostToNeighbor < neighbor->gCost || !inOpenList) {
				neighbor->gCost = newMovementCostToNeighbor;
				neighbor->hCost = (float)(abs(targetX - checkX) + abs(targetY - checkY));
				neighbor->parent = current;

				if (!inOpenList) {
					openList.push_back(neighbor);
				}
			}
		}
	}

	return path; // 未找到路径，返回空
}

// ======================================================================================
// 4. 视线检测 (Raycast)
// ======================================================================================

bool Pathfinder::RaycastHit(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end)
{
	// 1. 计算向量与距离
	float dx = end.x - start.x;
	float dz = end.z - start.z;
	float dist = sqrtf(dx * dx + dz * dz);

	if (dist < 0.1f) return false; // 距离太近，认为没有遮挡

	// 2. 步进检测 (每隔 0.5 米采样一次)
	// 步长越小越精确，但开销越大。0.5f 通常是小于格子大小的安全值。
	float stepSize = 0.5f;
	int steps = (int)(dist / stepSize);

	float stepX = (dx / dist) * stepSize;
	float stepZ = (dz / dist) * stepSize;

	// 3. 逐步检测
	float curX = start.x;
	float curZ = start.z;

	for (int i = 0; i < steps; i++)
	{
		curX += stepX;
		curZ += stepZ;

		int gx, gy;
		WorldToGrid(curX, curZ, gx, gy);

		// 越界检查
		if (gx < 0 || gx >= GRID_WIDTH || gy < 0 || gy >= GRID_HEIGHT) {
			continue; // 如果越界通常视为墙壁，或者根据需求返回 true
		}

		// 撞墙了
		if (m_Grid[gx][gy].isObstacle) {
			return true;
		}
	}

	// 一路畅通
	return false;
}
