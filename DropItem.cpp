#include "DropItem.h"
#include "cube.h"       
#include "texture.h"
#include "collision.h"  
#include "PlayerCharacter.h"
#include "billboard.h"
#include "Inventory.h"  
#include <vector>
#include "Meshfield.h"
#include "Shader_Shadow.h"
#include "model.h"
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
	g_DropTexID = Texture_LoadFromFile(L"resource/texture/AmmoBox.png");

	// 如果没找到，就用白色
	if (g_DropTexID == -1) g_DropTexID = Texture_LoadFromFile(L"resource/texture/white.png");
}

void DropItem_Finalize()
{
}

void DropItem_Spawn(XMFLOAT3 position, int itemId)
{
	for (int i = 0; i < MAX_DROPS; i++)
	{
		if (!g_Drops[i].active)
		{
			g_Drops[i].active = true;
			g_Drops[i].position = position;
			g_Drops[i].position.y -= 0.5f;
			g_Drops[i].itemId = itemId;
			g_Drops[i].floatAngle = 0.0f;
			g_Drops[i].rotationY = 0.0f;
			return;
		}
	}
}

void DropItem_DrawShadow(const DirectX::XMMATRIX& lightView, const DirectX::XMMATRIX& lightProj)
{
	// 1. 【绝对关键】强制恢复阴影 Shader 和 光栅化状态(CullMode=None)
	// 这能解决因状态残留导致的闪烁问题
	Shader_Shadow_Apply();

	for (int i = 0; i < MAX_DROPS; i++)
	{
		if (!g_Drops[i].active) continue;

		DropItem& item = g_Drops[i];

		// 2. 构建矩阵：缩放 -> 旋转 -> 位移
		// 这里的 rotationY 会让阴影跟随旋转
		DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);
		DirectX::XMMATRIX rot = DirectX::XMMatrixRotationY(item.rotationY);
		DirectX::XMMATRIX trans = DirectX::XMMatrixTranslation(item.position.x, item.position.y, item.position.z);

		DirectX::XMMATRIX world = scale * rot * trans;

		// 3. 绘制
		Cube_DrawShadow(world);
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

		// 1. 更新动画
		item.rotationY += 2.0f * (float)elapsed_time; // 旋转
		item.floatAngle += 3.0f * (float)elapsed_time; // 漂浮

		// 计算漂浮高度
		// Update 里直接修改 position.y，这样绘制和碰撞检测都会使用最新的高度
		float floatOffset = sinf(item.floatAngle) * 0.2f;
		float groundHeight = MeshField_GetHeight(item.position.x, item.position.z);
		float baseOffset = 0.5f;
		item.position.y = groundHeight + baseOffset + floatOffset;


		// 2. 拾取检测 (修正了参数不匹配和逻辑混乱的问题)
		// 注意：这里需要给 playerPos 补一个半径 (0.5f) 构成球体，否则会报错
		if (Collision_IsOverlapSphere({ item.position, pickupRange }, { playerPos, 0.5f }))
		{
			// 分流处理：子弹直接吃，道具进背包
			if (item.itemId == 4)
			{
				Player_AddAmmo(30);
				item.active = false; // 直接销毁
			}
			else
			{
				// 尝试放入背包
				bool success = Inventory_AddItem(item.itemId, 1);
				if (success)
				{
					item.active = false; // 只有放进去了才销毁
				}
			}
		}
	}
}

void DropItem_Draw()
{
	// ==========================================
	// [已启用] 方块渲染模式 (测试用)
	// ==========================================
	for (int i = 0; i < MAX_DROPS; i++)
	{
		if (!g_Drops[i].active) continue;

		DropItem& item = g_Drops[i];

		// 构建世界矩阵
		// 注意：因为 Update 里已经把 floatOffset 算进 item.position.y 了，
		// 这里直接用 item.position 即可，不要再加 sinf 了，否则会鬼畜。
		XMMATRIX scale = XMMatrixScaling(0.5f, 0.5f, 0.5f); // 0.5米的小方块
		XMMATRIX rot = XMMatrixRotationY(item.rotationY);
		XMMATRIX trans = XMMatrixTranslation(item.position.x, item.position.y, item.position.z);

		XMMATRIX world = scale * rot * trans;

		// 绘制方块
		Cube_Draw(g_DropTexID, world);
	}

	// ==========================================
	// [已注释] 广告牌渲染模式
	// ==========================================
	/*
	int texID = Inventory_GetIconsTextureID();
	if (texID == -1) return;

	for (int i = 0; i < MAX_DROPS; i++)
	{
		if (!g_Drops[i].active) continue;
		DropItem& item = g_Drops[i];

		int uvIndex = Inventory_GetItemUVIndex(item.itemId);
		float iconSize = 32.0f;
		float textureSize = 256.0f;
		float u = (uvIndex * iconSize) / textureSize;
		float v = 0.0f;
		float uw = iconSize / textureSize;
		float vh = iconSize / textureSize;

		// Update 里已经更新了 Y，这里直接画
		Billboard_Draw(texID, item.position, 1.0f, 1.0f, u, v, uw, vh);
	}
	*/
}