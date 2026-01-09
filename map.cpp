#include "map.h"
#include "collision.h"
#include "model.h"
#include "cube.h"
#include "texture.h"
#include "Pathfinder.h"
#include <DirectXMath.h>
#include "Light.h"
#include "Meshfield.h"
#include "Player_Camera.h"
#include "shader_3d.h"
#include <vector>
#include <cstdlib>
#include <ctime>  
using namespace DirectX;

namespace {
	// 仅保留地图对象容器
	std::vector<MapObject> g_MapObjects;
	int g_WallTextureID = -1;
}

static void AddWall(float x, float z, float y = 0.5f)
{
	MapObject wall;
	wall.KindId = MAP_KIND_WALL;

	// 使用传入的 y 坐标
	wall.Position = { x, y, z };

	// 创建物理碰撞盒
	wall.Aabb = Cube_CreateAABB(wall.Position);

	// 添加到列表
	g_MapObjects.push_back(wall);

	// 【重要】Pathfinder 是 2D 网格寻路，无论堆多高，只要有箱子，这个 (x,z) 就是障碍
	Pathfinder::SetObstacle(x, z, true);
}
void Map_Initialize()
{
	// 1. 清理
	g_MapObjects.clear();
	g_WallTextureID = Texture_LoadFromFile(L"resource/texture/Stone.png");

	// 2. 地面
	MapObject ground;
	ground.KindId = MAP_KIND_GROUND;
	ground.Position = { 0.0f, 0.0f, 0.0f };
	float w = MeshField_GetWidth() / 2.0f;
	float d = MeshField_GetDepth() / 2.0f;
	ground.Aabb = { {-w, -1.0f, -d}, {w, 0.0f, d} };
	g_MapObjects.push_back(ground);

	// 3. 随机生成障碍物
	srand((unsigned int)time(NULL)); // 设置随机种子

	int obstacleCount = 60;   // 障碍物数量
	float range = 25.0f;      // 分布范围
	float safeRadius = 3.0f;  // 出生点保护半径

	for (int i = 0; i < obstacleCount; i++)
	{
		// 生成随机坐标并取整
		float x = (float)((rand() % (int)(range * 2)) - range);
		float z = (float)((rand() % (int)(range * 2)) - range);

		// 避开玩家出生点
		if (abs(x) < safeRadius && abs(z) < safeRadius) {
			i--; continue;
		}

		// 避开终点位置 (假设终点在 10,10)
		if (abs(x - 10.0f) < 2.0f && abs(z - 10.0f) < 2.0f) {
			i--; continue;
		}

		// 生成底座
		AddWall(x, z, 0.5f);

		// 30% 概率叠加第二层
		if ((rand() % 100) < 30) {
			AddWall(x, z, 1.5f);

			// 再 30% 概率叠加第三层
			if ((rand() % 100) < 30) {
				AddWall(x, z, 2.5f);
			}
		}
	}
}

void Map_Finalize()
{
	g_MapObjects.clear();
}

void Map_Draw()
{
	XMMATRIX mtxWorld;

	for (const MapObject& o : g_MapObjects) {
		switch (o.KindId) {
		case 0: // 仅渲染地面
			mtxWorld = XMMatrixIdentity();
			// 设置地面反光属性
			Light_SetSpecularWorld(Player_Camera_GetPosition(), 1.0f, { 0.1f, 0.1f, 0.1f, 1.0f });
			MeshField_Draw(mtxWorld);
			// 恢复默认反光强度
			Light_SetSpecularWorld(Player_Camera_GetPosition(), 10.0f, { 0.8f, 0.8f, 0.8f, 1.0f });
			break;
		case MAP_KIND_WALL:
			// 绘制墙壁 (Cube)
			// 计算世界矩阵：平移
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);

			// 如果想把墙变红，可以在这里 SetColor，画完再 SetColor(White)
			Shader_3D_SetColor({ 1.0f, 0.5f, 0.5f, 1.0f });
			Cube_Draw(g_WallTextureID, mtxWorld);
			Shader_3D_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 恢复白色
			break;
		default:
			break;
		}
	}
}

int Map_GetObjectsCount()
{
	return (int)g_MapObjects.size();
}

const MapObject* Map_GetObject(int index)
{
	if (index < 0 || index >= (int)g_MapObjects.size()) {
		return nullptr;
	}
	return &g_MapObjects[index];
}

const std::vector<MapObject>& Map_GetObjects()
{
	return g_MapObjects;
}

bool Map_CheckCollision(const AABB& objAabb)
{
	for (const auto& obj : g_MapObjects)
	{
		// 只检测墙壁，不检测地面 (KindId == 1)
		if (obj.KindId == MAP_KIND_WALL)
		{
			if (Collision_IsOverLapAABB(objAabb, obj.Aabb))
			{
				return true; // 撞到了
			}
		}
	}
	return false;
}

bool Map_CheckLineOfSightBlocked(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end)
{
	Ray ray;
	ray.origin = start;

	XMVECTOR vStart = XMLoadFloat3(&start);
	XMVECTOR vEnd = XMLoadFloat3(&end);
	XMVECTOR vDir = vEnd - vStart;
	float distToTarget = XMVectorGetX(XMVector3Length(vDir)); // 目标距离

	vDir = XMVector3Normalize(vDir);
	XMStoreFloat3(&ray.direction, vDir);

	for (const auto& obj : g_MapObjects)
	{
		if (obj.KindId == MAP_KIND_WALL)
		{
			float hitDist = 0.0f;
			if (Collision_IntersectRayAABB(ray, obj.Aabb, hitDist))
			{
				// 如果撞墙距离 < 目标距离，说明视线被阻挡
				if (hitDist < distToTarget) {
					return true;
				}
			}
		}
	}
	return false;
}