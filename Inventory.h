#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>
// 道具类型
enum class ItemType {
	Consumable, // 消耗品 (药水)
	Equipment,  // 装备 (武器/防具)
	KeyItem,    // 关键道具 (钥匙)
	Material    // 材料
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

// 背包格子 (玩家实际持有的数据)
struct InventorySlot {
	int itemId; // 持有的道具ID (-1 表示空)
	int count;  // 持有的数量
	bool isEmpty() const { return itemId == -1 || count <= 0; }
};

// --- 系统接口 ---

// 初始化背包系统
void Inventory_Initialize();

// 更新背包 (处理输入：开关背包、移动光标、使用道具)
void Inventory_Update(double elapsed_time);

// 绘制背包 (UI层)
void Inventory_Draw();

// 销毁
void Inventory_Finalize();

// --- 功能接口 (给 Game 或 Player 调用) ---

// 添加道具 (返回是否成功)
bool Inventory_AddItem(int itemId, int count);

// 移除道具
bool Inventory_RemoveItem(int slotIndex, int count);

// 检查背包是否打开
bool Inventory_IsOpen();

int Inventory_GetItemUVIndex(int itemId);

int Inventory_GetIconsTextureID();