#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>
// 道具类型
enum class ItemType {
	Consumable,
	Equipment,
	KeyItem,
	Material
};

// 道具的基础定义 (只读数据，类似数据库)
struct ItemDefinition {
	int id;             // 道具ID
	std::wstring name;  // 道具名称
	std::wstring desc;  // 道具描述
	ItemType type;      // 类型
	int maxStack;       // 最大堆叠数量 (例如药水99，武器1)
	int textureId;      // 图标纹理ID
	int uvIndex;        // 图标在图集中的位置 (如果不使用图集，可以忽略)
};

struct InventorySlot {
	int itemId;
	int count;
	bool isEmpty() const { return itemId == -1 || count <= 0; }
};
void Inventory_Initialize();
void Inventory_Update(double elapsed_time);
void Inventory_Draw();
void Inventory_Finalize();
bool Inventory_AddItem(int itemId, int count);
bool Inventory_RemoveItem(int slotIndex, int count);
bool Inventory_IsOpen();
int Inventory_GetItemUVIndex(int itemId);
void UI_DrawHUD();
int Inventory_GetIconsTextureID();