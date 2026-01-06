#include "Pathfinder.h"
#include <cmath>
#include <algorithm>
#include <vector>

using namespace DirectX;

Node Pathfinder::m_Grid[GRID_WIDTH][GRID_HEIGHT];

void Pathfinder::Initialize() {
	// 初始化网格，默认为空地
	for (int x = 0; x < GRID_WIDTH; x++) {
		for (int y = 0; y < GRID_HEIGHT; y++) {
			m_Grid[x][y].x = x;
			m_Grid[x][y].y = y;
			m_Grid[x][y].isObstacle = false;
		}
	}
}

void Pathfinder::Finalize() {
	// 静态数组不需要特殊释放
}

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
		gx * GRID_SIZE + MAP_OFFSET_X + GRID_SIZE * 0.5f, // 加0.5让点在格子中心
		0.0f,
		gy * GRID_SIZE + MAP_OFFSET_Z + GRID_SIZE * 0.5f
	};
}

std::vector<XMFLOAT3> Pathfinder::FindPath(XMFLOAT3 startPos, XMFLOAT3 targetPos) {
	std::vector<XMFLOAT3> path;

	// 1. 转换坐标
	int startX, startY, targetX, targetY;
	WorldToGrid(startPos.x, startPos.z, startX, startY);
	WorldToGrid(targetPos.x, targetPos.z, targetX, targetY);

	// 边界检查
	if (startX < 0 || startX >= GRID_WIDTH || startY < 0 || startY >= GRID_HEIGHT ||
		targetX < 0 || targetX >= GRID_WIDTH || targetY < 0 || targetY >= GRID_HEIGHT) {
		return path; // 超出地图，无法寻路
	}

	// 如果终点本身是障碍物，或者起点就是终点，直接返回
	if (m_Grid[targetX][targetY].isObstacle || (startX == targetX && startY == targetY)) {
		return path;
	}

	// 2. 重置寻路数据
	for (int x = 0; x < GRID_WIDTH; x++) {
		for (int y = 0; y < GRID_HEIGHT; y++) {
			m_Grid[x][y].gCost = 99999.0f;
			m_Grid[x][y].hCost = 0.0f;
			m_Grid[x][y].parent = nullptr;
			m_Grid[x][y].closed = false;
		}
	}

	// 3. 开启列表 (这里用简单的 list，性能要求高可以用 priority_queue)
	std::vector<Node*> openList;

	Node* startNode = &m_Grid[startX][startY];
	startNode->gCost = 0;
	// H值计算 (曼哈顿距离：绝对值之和，计算快)
	startNode->hCost = (float)(abs(targetX - startX) + abs(targetY - startY));
	openList.push_back(startNode);

	while (!openList.empty()) {
		// --- 3.1 找出 F 值最小的节点 ---
		Node* current = openList[0];
		int currentIndex = 0;
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

		// --- 3.2 到达终点？ ---
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

		// --- 3.3 检查邻居 (上下左右 4个方向) ---
		// 如果想斜着走，需要检查 8 个方向
		int dx[4] = { 0, 0, 1, -1 };
		int dy[4] = { 1, -1, 0, 0 };

		for (int i = 0; i < 4; i++) {
			int checkX = current->x + dx[i];
			int checkY = current->y + dy[i];

			// 越界检查
			if (checkX < 0 || checkX >= GRID_WIDTH || checkY < 0 || checkY >= GRID_HEIGHT) continue;

			Node* neighbor = &m_Grid[checkX][checkY];

			// 障碍物或已关闭检查
			if (neighbor->isObstacle || neighbor->closed) continue;

			// 计算新的 G 值 (邻居距离为1)
			float newMovementCostToNeighbor = current->gCost + 1.0f;

			// 如果这条路更短，或者邻居不在 OpenList 中
			bool inOpenList = false;
			for (auto* n : openList) if (n == neighbor) inOpenList = true;

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

	return path; // 没找到路径，返回空
}

bool Pathfinder::RaycastHit(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end)
{
	// 1. 计算方向和距离
	float dx = end.x - start.x;
	float dz = end.z - start.z;
	float dist = sqrtf(dx * dx + dz * dz);

	if (dist < 0.1f) return false; // 就在脸上了，肯定没墙

	// 2. 步长设置 (例如每 0.5 米检查一次)
	// 步长越小越精确，但消耗越大。0.5f 通常刚好（小于一个格子大小）
	float stepSize = 0.5f;
	int steps = (int)(dist / stepSize);

	float stepX = (dx / dist) * stepSize;
	float stepZ = (dz / dist) * stepSize;

	// 3. 逐步检查
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
			continue; // 或者 return true (视为墙)
		}

		// 撞墙了！
		if (m_Grid[gx][gy].isObstacle) {
			return true;
		}
	}

	// 一路通畅
	return false;
}
