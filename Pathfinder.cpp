#include "Pathfinder.h"
#include <cmath>
#include <algorithm>
#include <vector>

using namespace DirectX;

Node Pathfinder::m_Grid[GRID_WIDTH][GRID_HEIGHT];

void Pathfinder::Initialize() {
	// ��ʼ������Ĭ��Ϊ�յ�
	for (int x = 0; x < GRID_WIDTH; x++) {
		for (int y = 0; y < GRID_HEIGHT; y++) {
			m_Grid[x][y].x = x;
			m_Grid[x][y].y = y;
			m_Grid[x][y].isObstacle = false;
		}
	}
}

void Pathfinder::Finalize() {
	// ��̬���鲻��Ҫ�����ͷ�
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
		gx * GRID_SIZE + MAP_OFFSET_X + GRID_SIZE * 0.5f, // ��0.5�õ��ڸ�������
		0.0f,
		gy * GRID_SIZE + MAP_OFFSET_Z + GRID_SIZE * 0.5f
	};
}

std::vector<XMFLOAT3> Pathfinder::FindPath(XMFLOAT3 startPos, XMFLOAT3 targetPos) {
	std::vector<XMFLOAT3> path;

	// 1. ת������
	int startX, startY, targetX, targetY;
	WorldToGrid(startPos.x, startPos.z, startX, startY);
	WorldToGrid(targetPos.x, targetPos.z, targetX, targetY);

	// �߽���
	if (startX < 0 || startX >= GRID_WIDTH || startY < 0 || startY >= GRID_HEIGHT ||
		targetX < 0 || targetX >= GRID_WIDTH || targetY < 0 || targetY >= GRID_HEIGHT) {
		return path; // ������ͼ���޷�Ѱ·
	}

	// ����յ㱾�����ϰ�������������յ㣬ֱ�ӷ���
	if (m_Grid[targetX][targetY].isObstacle || (startX == targetX && startY == targetY)) {
		return path;
	}

	// 2. ����Ѱ·����
	for (int x = 0; x < GRID_WIDTH; x++) {
		for (int y = 0; y < GRID_HEIGHT; y++) {
			m_Grid[x][y].gCost = 99999.0f;
			m_Grid[x][y].hCost = 0.0f;
			m_Grid[x][y].parent = nullptr;
			m_Grid[x][y].closed = false;
		}
	}

	// 3. �����б� (�����ü򵥵� list������Ҫ��߿����� priority_queue)
	std::vector<Node*> openList;

	Node* startNode = &m_Grid[startX][startY];
	startNode->gCost = 0;
	// Hֵ���� (�����پ��룺����ֵ֮�ͣ������)
	startNode->hCost = (float)(abs(targetX - startX) + abs(targetY - startY));
	openList.push_back(startNode);

	while (!openList.empty()) {
		// --- 3.1 �ҳ� F ֵ��С�Ľڵ� ---
		Node* current = openList[0];
		int currentIndex = 0;
		for (size_t i = 1; i < openList.size(); i++) {
			if (openList[i]->FCost() < current->FCost() ||
				(openList[i]->FCost() == current->FCost() && openList[i]->hCost < current->hCost)) {
				current = openList[i];
				currentIndex = i;
			}
		}

		// �Ƴ� OpenList������ ClosedList
		openList.erase(openList.begin() + currentIndex);
		current->closed = true;

		// --- 3.2 �����յ㣿 ---
		if (current->x == targetX && current->y == targetY) {
			// ����·��
			Node* curr = current;
			while (curr != nullptr) {
				path.push_back(GridToWorld(curr->x, curr->y));
				curr = curr->parent;
			}
			// ·���Ƿ��� (�յ�->���)����Ҫ��ת
			std::reverse(path.begin(), path.end());
			return path;
		}

		// --- 3.3 ����ھ� (�������� 4������) ---
		// �����б���ߣ���Ҫ��� 8 ������
		int dx[4] = { 0, 0, 1, -1 };
		int dy[4] = { 1, -1, 0, 0 };

		for (int i = 0; i < 4; i++) {
			int checkX = current->x + dx[i];
			int checkY = current->y + dy[i];

			// Խ����
			if (checkX < 0 || checkX >= GRID_WIDTH || checkY < 0 || checkY >= GRID_HEIGHT) continue;

			Node* neighbor = &m_Grid[checkX][checkY];

			// �ϰ�����ѹرռ��
			if (neighbor->isObstacle || neighbor->closed) continue;

			// �����µ� G ֵ (�ھӾ���Ϊ1)
			float newMovementCostToNeighbor = current->gCost + 1.0f;

			// �������·���̣������ھӲ��� OpenList ��
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

	return path; // û�ҵ�·�������ؿ�
}

bool Pathfinder::RaycastHit(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end)
{
	// 1. ���㷽��;���
	float dx = end.x - start.x;
	float dz = end.z - start.z;
	float dist = sqrtf(dx * dx + dz * dz);

	if (dist < 0.1f) return false; // ���������ˣ��϶�ûǽ

	// 2. �������� (����ÿ 0.5 �׼��һ��)
	// ����ԽСԽ��ȷ��������Խ��0.5f ͨ���պã�С��һ�����Ӵ�С��
	float stepSize = 0.5f;
	int steps = (int)(dist / stepSize);

	float stepX = (dx / dist) * stepSize;
	float stepZ = (dz / dist) * stepSize;

	// 3. �𲽼��
	float curX = start.x;
	float curZ = start.z;

	for (int i = 0; i < steps; i++)
	{
		curX += stepX;
		curZ += stepZ;

		int gx, gy;
		WorldToGrid(curX, curZ, gx, gy);

		// Խ����
		if (gx < 0 || gx >= GRID_WIDTH || gy < 0 || gy >= GRID_HEIGHT) {
			continue; // ���� return true (��Ϊǽ)
		}

		// ײǽ�ˣ�
		if (m_Grid[gx][gy].isObstacle) {
			return true;
		}
	}

	// һ·ͨ��
	return false;
}
