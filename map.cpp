#include "map.h"
#include "collision.h"
#include "model.h"
#include "cube.h"
#include "texture.h"
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
	std::vector<MapObject> g_MapObjects;

	int g_CubeTexID = -1;
	MODEL* g_Model = nullptr;

	static const float MODEL_OFFSET_X = 5.0f;
	static const float TREE_Y_ON_GROUND = 0.0f;
}

void Map_Initialize()
{
	g_CubeTexID = Texture_LoadFromFile(L"resource/texture/Cube_Draw.png");
	g_Model = ModelLoad("resource/Model/Tree.fbx", 0.5f);

	srand((unsigned int)time(nullptr));

	// 1. 定义初始的固定物体
	// 为了避免混淆，这里直接填入修正后的最终坐标
	MapObject initial_data[] = {
		// 地面
		{ 0, { 0.0f, 0.0f, 0.0f }, { { -25.0f, -1.0f, -12.5f}, {25.0f, 0.0f, 12.5f}}},

		// 箱子
		{ 1, { 1.0f,0.5f,0.0f} },
		{ 1, {-1.0f,0.5f,0.0f} },
		{ 1, { 0.0f,0.5f,0.0f} },
		{ 1, { 1.0f,0.5f,1.0f} },
		{ 1, {-1.0f,0.5f,1.0f} },
		{ 1, { 1.0f,0.5f,2.0f} },
		{ 1, { 0.0f,1.5f,2.0f} },
		{ 1, {-1.0f,1.5f,2.0f} },

		// 手动放置的树 (已手动应用了之前的 -4.5 修正，并加上了 X+5 的偏移)
		// 原 8.5 -> 改为 4.0 (站在地面)
		// 原 6.5 -> 改为 2.0 (沉入地下一点)
		// 原 5.5 -> 改为 1.0
		// 原 4.5 -> 改为 0.0
		// X 坐标也加上 5.0 的偏移 (-1.0 + 5.0 = 4.0)
		{ 2, { 4.0f, TREE_Y_ON_GROUND, 2.0f} },
		{ 2, { 6.0f, TREE_Y_ON_GROUND, 2.0f} },
		{ 2, { 7.0f, TREE_Y_ON_GROUND, 2.0f} },
		{ 2, { 8.0f, TREE_Y_ON_GROUND, 2.0f} },
	};

	// 2. 将固定物体加入列表
	for (auto& item : initial_data)
	{
		// 动态计算 AABB
		if (item.KindId == 0) { // 地面
			float w = MeshField_GetWidth() / 2.0f;
			float d = MeshField_GetDepth() / 2.0f;
			item.Aabb = { {-w, -1.0f, -d}, {w, 0.0f, d} };
		}
		else if (item.KindId == 2) {
			// 树木中心在 Y=4.0，高度约为 8.0，宽度设为 1.5 (半径0.75)
			float halfW = 0.5f;
			float halfH = 2.0f;
			float halfD = 0.5f;

			// 手动构建一个落地的大包围盒
			item.Aabb.min = { item.Position.x - halfW, item.Position.y,          item.Position.z - halfD };
			item.Aabb.max = { item.Position.x + halfW, item.Position.y + halfH, item.Position.z + halfD };
		}
		else { // 箱子 (KindId == 1) 继续用默认的小方块
			item.Aabb = Cube_CreateAABB(item.Position);
		}
		g_MapObjects.push_back(item);
	}

	// 3. 批量生成随机树木
	int randomTreeCount = 40;
	float groundW = MeshField_GetWidth() / 3.0f;
	float range = groundW * 0.8f; // 范围稍微比地面小一点

	for (int i = 0; i < randomTreeCount; i++)
	{
		MapObject tree;
		tree.KindId = 2; // 树

		float randX = (float)(rand() % (int)(range * 20)) / 10.0f - range;
		float randZ = (float)(rand() % (int)(range * 20)) / 10.0f - range;

		// 应用坐标
		tree.Position.x = randX + MODEL_OFFSET_X;
		tree.Position.y = TREE_Y_ON_GROUND; // 0.0f
		tree.Position.z = randZ;

		// [修改] 设置随机树木的 AABB (与上面逻辑一致)
		float halfW = 0.5f;
		float height = 2.0f;
		float halfD = 0.5f;

		// 确保 Y 轴范围是从 0 到 8
		tree.Aabb.min = { tree.Position.x - halfW, tree.Position.y,          tree.Position.z - halfD };
		tree.Aabb.max = { tree.Position.x + halfW, tree.Position.y + height, tree.Position.z + halfD };

		g_MapObjects.push_back(tree);
	}
}

void Map_Finalize()
{
	ModelRelease(g_Model);
	g_MapObjects.clear();
}

void Map_Draw()
{
	XMMATRIX mtxWorld;

	for (const MapObject& o : g_MapObjects) {
		switch (o.KindId) {
		case 0: // 地面
			mtxWorld = XMMatrixIdentity();
			Light_SetSpecularWorld(Player_Camera_GetPosition(), 1.0f, { 0.1f,0.1f,0.1f,1.0f });
			MeshField_Draw(mtxWorld);
			Light_SetSpecularWorld(Player_Camera_GetPosition(), 10.0f, { 0.8f,0.8f,0.8f,1.0f });
			break;

		case 1: // 方块
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_Draw(g_CubeTexID, mtxWorld);
			break;

		case 2: // 树木
		{
			// 直接使用存储的坐标，不再进行任何加减
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);

			if (g_Model) {
				ModelDraw(g_Model, mtxWorld);
			}
		}
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