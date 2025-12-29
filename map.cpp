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

using namespace DirectX;

namespace {
	// 仅保留地图对象容器
	std::vector<MapObject> g_MapObjects;

	// 删除了 g_Model (树木模型) 和所有相关偏移常量
}

void Map_Initialize()
{
	// 1. 清空旧数据
	g_MapObjects.clear();

	// 2. 仅创建地面对象 (KindId: 0)
	MapObject ground;
	ground.KindId = 0;
	ground.Position = { 0.0f, 0.0f, 0.0f };

	// 根据 MeshField 的尺寸计算地面的 AABB 碰撞盒
	float w = MeshField_GetWidth() / 2.0f;
	float d = MeshField_GetDepth() / 2.0f;
	ground.Aabb = { {-w, -1.0f, -d}, {w, 0.0f, d} };

	g_MapObjects.push_back(ground);

	// --- 所有的 KindId == 2 (树木) 的硬编码数据和随机生成逻辑已全部删除 ---
}

void Map_Finalize()
{
	// 删除了 ModelRelease(g_Model)
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

			// case 1 (箱子) 和 case 2 (树木) 的绘制逻辑已全部删除
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