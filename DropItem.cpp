#include "DropItem.h"
#include "cube.h"        // 用方块代表道具
#include "texture.h"
#include "collision.h"   // 碰撞检测
#include "Player.h"      // 获取玩家位置
#include "billboard.h"
#include "Inventory.h"   // 放入背包
#include <vector>

using namespace DirectX;

// 定义单个掉落物的数据结构
struct DropItem
{
	bool active;
	XMFLOAT3 position;
	int itemId;          // 这个掉落物对应哪个道具 (0=药水, 1=剑...)
	float floatAngle;    // 用于上下漂浮动画的角度
	float rotationY;     // 用于自转的角度
};

namespace
{
	const int MAX_DROPS = 100;
	DropItem g_Drops[MAX_DROPS];

	int g_DropTexID = -1; // 掉落物外观贴图
}

void DropItem_Initialize()
{
	// 初始化池
	for (int i = 0; i < MAX_DROPS; i++) {
		g_Drops[i].active = false;
	}

	// 加载一个贴图作为掉落物外观
	// 既然我们之前生成了金色的 ui_cursor.png，它很像一个宝箱框，正好拿来用！
	// 或者你可以专门画一个 "Chest.png"
	g_DropTexID = Texture_LoadFromFile(L"resource/texture/ui_cursor.png");

	// 如果没找到，就用白色
	if (g_DropTexID == -1) g_DropTexID = Texture_LoadFromFile(L"resource/texture/white.png");
}

void DropItem_Finalize()
{
	// 不需要特殊清理，除非有动态申请的内存
}

void DropItem_Spawn(XMFLOAT3 position, int itemId)
{
	// 找一个空闲的坑位
	for (int i = 0; i < MAX_DROPS; i++)
	{
		if (!g_Drops[i].active)
		{
			g_Drops[i].active = true;
			g_Drops[i].position = position;
			g_Drops[i].position.y -= 0.5f; // 稍微抬高一点，不要埋在地里
			g_Drops[i].itemId = itemId;
			g_Drops[i].floatAngle = 0.0f;
			g_Drops[i].rotationY = 0.0f;
			return;
		}
	}
}

void DropItem_Update(double elapsed_time)
{
	XMFLOAT3 playerPos = Player_GetPosition();
	float pickupRange = 1.0f; // 拾取半径

	for (int i = 0; i < MAX_DROPS; i++)
	{
		if (!g_Drops[i].active) continue;

		DropItem& item = g_Drops[i];

		// 1. 动画效果：自转 + 上下漂浮
		item.rotationY += 2.0f * (float)elapsed_time; // 旋转速度
		item.floatAngle += 3.0f * (float)elapsed_time; // 漂浮速度

		// 简单的上下漂浮偏移 (Sin波)
		float floatOffset = sinf(item.floatAngle) * 0.2f;

		// 2. 检测与玩家的碰撞 (拾取逻辑)
		// 这种简单的距离检测就够了
		if (Collision_IsOverlapSphere({ item.position, pickupRange }, playerPos))
		{
			// [关键] 尝试加入背包
			// 这里我们假设添加1个
			bool success = Inventory_AddItem(item.itemId, 1);

			if (success)
			{
				// 成功捡起：销毁地上模型
				item.active = false;

				// 可以在这里播放一个音效或者特效
				// PlaySound(...);
			}
			else
			{
				// 背包满了？提示一下 (可选)
				// DrawDebugText(..., "Inventory Full!");
			}
		}
	}
}

void DropItem_Draw()
{
	//for (int i = 0; i < MAX_DROPS; i++)
	//{
	//	if (!g_Drops[i].active) continue;

	//	DropItem& item = g_Drops[i];

	//	// 计算漂浮后的 Y 坐标
	//	float currentY = item.position.y + sinf(item.floatAngle) * 0.2f;

	//	// 构建世界矩阵：缩放 -> 旋转 -> 平移
	//	XMMATRIX scale = XMMatrixScaling(0.5f, 0.5f, 0.5f); // 道具做小一点 (0.5米)
	//	XMMATRIX rot = XMMatrixRotationY(item.rotationY);
	//	XMMATRIX trans = XMMatrixTranslation(item.position.x, currentY, item.position.z);

	//	XMMATRIX world = scale * rot * trans;

	//	// 绘制方块
	//	Cube_Draw(g_DropTexID, world);
	//}
	// 获取图集纹理 ID
	int texID = Inventory_GetIconsTextureID();
	if (texID == -1) return;

	for (int i = 0; i < MAX_DROPS; i++)
	{
		if (!g_Drops[i].active) continue;

		DropItem& item = g_Drops[i];

		// 1. 计算漂浮动画 (Y轴上下移动)
		// sinf 返回 -1~1，乘 0.2f 变成 -0.2~0.2
		float currentY = item.position.y + 0.5f + sinf(item.floatAngle) * 0.2f;
		XMFLOAT3 drawPos = { item.position.x, currentY, item.position.z };

		// 2. 获取 UV 信息
		int uvIndex = Inventory_GetItemUVIndex(item.itemId);

		// 假设图集 ui_icons.png 是 256x256，每个图标 32x32
		// 一行有 8 个图标 (256/32 = 8)
		float iconSize = 32.0f;
		float textureSize = 256.0f;

		// 计算 UV 比例 (0.0 ~ 1.0)
		float u = (uvIndex * iconSize) / textureSize;
		float v = 0.0f; // 目前只有第一行
		float uw = iconSize / textureSize; // 宽度比例 (32/256 = 0.125)
		float vh = iconSize / textureSize; // 高度比例

		// 3. 绘制 Billboard
		// 参数: 纹理ID, 位置, 宽度(1.0米), 高度(1.0米), u, v, uw, vh
		Billboard_Draw(texID, drawPos, 1.0f, 1.0f, u, v, uw, vh);
	}
}