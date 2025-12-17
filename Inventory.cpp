#include "Inventory.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"    // 用于输入控制
#include "key_logger.h"
#include "direct3d.h"    // 获取屏幕宽高
#include "debug_text.h"  // 暂时用 DebugText 显示文字，后期可以换专门的 Font
#include <map>
#include <cstdio>  
#include <cstdarg> 

using namespace DirectX;

namespace
{
	// --- 配置参数 ---
	const int MAX_SLOTS = 20;     // 背包最大格子数
	const int COLS = 5;           // 一行显示几个

	// --- 状态变量 ---
	bool g_IsOpen = false;        // 背包是否打开
	int g_CursorIndex = 0;        // 当前选中的格子索引 (0 ~ MAX_SLOTS-1)

	// --- 资源 ID ---
	int g_TexBackground = -1;     // 背包背景图
	int g_TexCursor = -1;         // 选中框图片
	int g_TexIcons = -1;          // 道具图标集合 (Sprite Sheet)

	int g_FontTexId = -1;        // DebugText 用的字体纹理

	// --- 数据存储 ---
	// 道具数据库 (ID -> 定义)
	std::map<int, ItemDefinition> g_ItemDatabase;

	// 玩家背包数据
	std::vector<InventorySlot> g_Inventory;
}

// 内部辅助：定义游戏里的道具 (相当于初始化数据库)
void DefineItems()
{
	// ID, Name, Desc, Type, MaxStack, TexID, UV
	g_ItemDatabase[0] = { 0, L"Health Potion", L"Restores 50 HP", ItemType::Consumable, 10, -1, 0 };
	g_ItemDatabase[1] = { 1, L"Iron Sword",    L"A rusty sword",  ItemType::Equipment,  1,  -1, 1 };
	g_ItemDatabase[2] = { 2, L"Magic Apple",   L"Grants XP",      ItemType::Consumable, 99, -1, 2 };
	g_ItemDatabase[3] = { 3, L"Dungeon Key",   L"Opens the gate", ItemType::KeyItem,    1,  -1, 3 };
}

void DrawDebugText(float x, float y, const char* fmt, ...)
{
	if (g_FontTexId == -1) return;

	// 1. 格式化字符串
	char buffer[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 256, fmt, args);
	va_end(args);

	// 2. 绘制参数设置
	// 假设 consolab_ascii_512.png 是 512x512 的图，包含 16x16 个字符
	// 每个字符在图上的尺寸是 32x32 像素
	float srcSize = 32.0f;

	// 屏幕上显示的尺寸 (可以调整这个值来缩放文字)
	float drawW = 16.0f; // 宽 (字宽通常比字高窄)
	float drawH = 32.0f; // 高

	float currentX = x;

	// 3. 遍历字符串绘制每个字符
	for (int i = 0; buffer[i] != '\0'; i++)
	{
		unsigned char c = buffer[i];

		// 跳过不可见字符
		if (c < 32) continue;

		// 计算字符在纹理中的网格位置 (ASCII 32 是起始的空格)
		int index = c - 32;
		int col = index % 16;
		int row = index / 16;

		float srcX = col * srcSize;
		float srcY = row * srcSize;

		// 调用 Sprite_Draw (使用支持源矩形裁剪的重载版本)
		// 参数: TextureID, DestX, DestY, DestW, DestH, SrcX, SrcY, SrcW, SrcH
		// 注意：如果你使用的 Sprite_Draw 没有这个重载，请参考 Score_Draw 的实现
		Sprite_Draw(g_FontTexId, currentX, y, drawW, drawH, srcX, srcY, srcSize, srcSize);

		currentX += drawW; // 光标后移
	}
}

void Inventory_Initialize()
{
	// 1. 初始化数据库
	DefineItems();

	// 2. 初始化背包格子 (全部设为空)
	g_Inventory.resize(MAX_SLOTS);
	for (auto& slot : g_Inventory) {
		slot.itemId = -1;
		slot.count = 0;
	}

	// 3. 加载 UI 图片
	// 请准备这些图片，或者暂时用白色方块代替
	g_TexBackground = Texture_LoadFromFile(L"resource/texture/ui_inventory_bg.png");
	if (g_TexBackground == -1) g_TexBackground = Texture_LoadFromFile(L"resource/texture/white.png");
	g_TexCursor = Texture_LoadFromFile(L"resource/texture/ui_cursor.png");
	if (g_TexCursor == -1) g_TexCursor = Texture_LoadFromFile(L"resource/texture/white.png");
	g_TexIcons = Texture_LoadFromFile(L"resource/texture/ui_icons.png");
	g_FontTexId = Texture_LoadFromFile(L"resource/texture/consolab_ascii_512.png");
	// 为了测试，我们默认给几个简单的白色纹理 (如果你没有图)

	// 4. [测试用] 开局送玩家一点东西
	Inventory_AddItem(0, 5); // 5瓶血药
	Inventory_AddItem(1, 1); // 1把剑
	Inventory_AddItem(1, 1);
	Inventory_AddItem(1, 1);
}

void Inventory_Update(double elapsed_time)
{
	// 1. 开关背包 (按 I 键)
	if (KeyLogger_IsTrigger(KK_I)) {
		g_IsOpen = !g_IsOpen;
	}

	if (!g_IsOpen) return;

	// 2. 移动光标 (方向键)
	if (KeyLogger_IsTrigger(KK_RIGHT)) {
		g_CursorIndex++;
		if (g_CursorIndex >= MAX_SLOTS) g_CursorIndex = 0; // 循环
	}
	if (KeyLogger_IsTrigger(KK_LEFT)) {
		g_CursorIndex--;
		if (g_CursorIndex < 0) g_CursorIndex = MAX_SLOTS - 1;
	}
	if (KeyLogger_IsTrigger(KK_UP)) {
		g_CursorIndex -= COLS;
		if (g_CursorIndex < 0) g_CursorIndex += MAX_SLOTS;
	}
	if (KeyLogger_IsTrigger(KK_DOWN)) {
		g_CursorIndex += COLS;
		if (g_CursorIndex >= MAX_SLOTS) g_CursorIndex -= MAX_SLOTS;
	}

	// 3. 使用道具 (按回车或空格)
	if (KeyLogger_IsTrigger(KK_ENTER)) {
		InventorySlot& slot = g_Inventory[g_CursorIndex];
		if (!slot.isEmpty()) {
			// 这里编写使用逻辑，例如加血
			// 目前仅打印日志或减少数量测试
			if (g_ItemDatabase[slot.itemId].type == ItemType::Consumable) {
				Inventory_RemoveItem(g_CursorIndex, 1);
			}
		}
	}
}

void Inventory_Draw()
{
	if (!g_IsOpen) return;
	Direct3D_SetBlendState(true);
	Direct3D_SetDepthEnable(false);

	// UI 参数
	float screenW = (float)Direct3D_GetBackBufferWidth();
	float screenH = (float)Direct3D_GetBackBufferHeight();

	// 居中显示背景面板
	float panelW = 400.0f;
	float panelH = 300.0f;
	float panelX = (screenW - panelW) / 2.0f;
	float panelY = (screenH - panelH) / 2.0f;

	// 1. 绘制背景
	// 颜色设为灰色半透明
	Sprite_Draw(g_TexBackground, panelX, panelY, panelW, panelH, { 0.2f, 0.2f, 0.2f, 0.9f });

	// 2. 绘制格子
	float slotSize = 50.0f;
	float gap = 10.0f; // 间距
	float startX = panelX + 30.0f;
	float startY = panelY + 30.0f;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		// 计算当前格子的屏幕坐标
		int col = i % COLS;
		int row = i / COLS;
		float x = startX + col * (slotSize + gap);
		float y = startY + row * (slotSize + gap);

		if (i == g_CursorIndex) {
			// 绘制格子底图 (深色)
			Sprite_Draw(g_TexBackground, x, y, slotSize, slotSize, { 0.0f, 0.0f, 0.0f, 0.5f });

			// 如果该格子有道具，绘制图标
			InventorySlot& slot = g_Inventory[i];

			if (!slot.isEmpty()) {
				ItemDefinition& def = g_ItemDatabase[slot.itemId];

				// 计算 UV 裁剪
				float iconRawSize = 32.0f;
				float srcX = (def.uvIndex * iconRawSize);
				float srcY = 0.0f;

				// [关键检查] 确保 g_TexIcons 是有效的
				if (g_TexIcons != -1) {
					Sprite_Draw(g_TexIcons, x + 4, y + 4, slotSize - 8, slotSize - 8, srcX, srcY, iconRawSize, iconRawSize);
				}

				// 绘制数量
				if (slot.count > 1) {
					DrawDebugText(x + 2, y + slotSize - 16, "%d", slot.count);
				}
			}

			// 4. 绘制选中框
			if (i == g_CursorIndex) {
				Sprite_Draw(g_TexCursor, x - 4, y - 4, slotSize + 8, slotSize + 8);

				// 2. 画道具描述 (在面板下方)
				if (!slot.isEmpty()) {
					ItemDefinition& def = g_ItemDatabase[slot.itemId];

					// Name
					std::string nameStr(def.name.begin(), def.name.end());
					DrawDebugText(panelX, panelY + panelH + 10, "Name: %s", nameStr.c_str());

					// Desc
					std::string descStr(def.desc.begin(), def.desc.end());
					DrawDebugText(panelX, panelY + panelH + 30, "Desc: %s", descStr.c_str());
				}
				else {
					DrawDebugText(panelX, panelY + panelH + 10, "Empty Slot");
				}
			}
		}
	}
}

void Inventory_Finalize()
{
	g_Inventory.clear();
	g_ItemDatabase.clear();
}

// --- 核心逻辑 ---

bool Inventory_AddItem(int itemId, int count)
{
	// 1. 先找能不能堆叠 (已有相同物品且未满)
	for (auto& slot : g_Inventory) {
		if (slot.itemId == itemId) {
			if (slot.count < g_ItemDatabase[itemId].maxStack) {
				int space = g_ItemDatabase[itemId].maxStack - slot.count;
				int add = (count < space) ? count : space;
				slot.count += add;
				count -= add;
				if (count <= 0) return true;
			}
		}
	}

	// 2. 找空位放剩下的
	for (auto& slot : g_Inventory) {
		if (slot.isEmpty()) {
			slot.itemId = itemId;
			slot.count = count;
			return true;
		}
	}

	return false; // 背包满了
}

bool Inventory_RemoveItem(int slotIndex, int count)
{
	if (slotIndex < 0 || slotIndex >= MAX_SLOTS) return false;

	InventorySlot& slot = g_Inventory[slotIndex];
	if (slot.isEmpty()) return false;

	slot.count -= count;
	if (slot.count <= 0) {
		slot.itemId = -1;
		slot.count = 0;
	}
	return true;
}

bool Inventory_IsOpen()
{
	return g_IsOpen;
}