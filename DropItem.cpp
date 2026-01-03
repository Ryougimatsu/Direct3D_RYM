#include "DropItem.h"
#include "cube.h"       
#include "texture.h"
#include "collision.h"  
#include "PlayerCharacter.h"
#include "billboard.h"
#include "Inventory.h"  
#include <vector>
#include "Meshfield.h"

using namespace DirectX;


struct DropItem
{
	bool active;
	XMFLOAT3 position;
	int itemId;         
	float floatAngle;   
	float rotationY;    
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

	
		item.rotationY += 2.0f * (float)elapsed_time; // 旋转速度
		item.floatAngle += 3.0f * (float)elapsed_time; // 漂浮速度

	
		float floatOffset = sinf(item.floatAngle) * 0.2f;

		float groundHeight = MeshField_GetHeight(item.position.x, item.position.z);
		float baseOffset = 0.5f;
		item.position.y = groundHeight + baseOffset + floatOffset;


		if (Collision_IsOverlapSphere({ item.position, pickupRange }, playerPos))
		{
	
			bool success = Inventory_AddItem(item.itemId, 1);

			if (success)
			{
				item.active = false;


			}
			else
			{

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

		
		float currentY = item.position.y + 0.5f + sinf(item.floatAngle) * 0.2f;
		XMFLOAT3 drawPos = { item.position.x, currentY, item.position.z };

		
		int uvIndex = Inventory_GetItemUVIndex(item.itemId);

		float iconSize = 32.0f;
		float textureSize = 256.0f;

	
		float u = (uvIndex * iconSize) / textureSize;
		float v = 0.0f; // 目前只有第一行
		float uw = iconSize / textureSize; // 宽度比例 (32/256 = 0.125)
		float vh = iconSize / textureSize; // 高度比例


		Billboard_Draw(texID, drawPos, 1.0f, 1.0f, u, v, uw, vh);
	}
}