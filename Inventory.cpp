#include "Inventory.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"   
#include "key_logger.h"
#include "direct3d.h"   
#include "debug_text.h" 
#include <map>
#include <cstdio>  
#include <cstdarg> 
#include "PlayerCharacter.h"

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
	float g_ItemUseCooldown = 0.0f;// 道具使用冷却时间
	int g_FontTexId = -1;        // DebugText 用的字体纹理



	std::map<int, ItemDefinition> g_ItemDatabase;
	std::vector<InventorySlot> g_Inventory;
}


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

	float srcSize = 32.0f;

	
	float drawW = 16.0f;
	float drawH = 32.0f;

	float currentX = x;

	// 3. 遍历字符串绘制每个字符
	for (int i = 0; buffer[i] != '\0'; i++)
	{
		unsigned char c = buffer[i];

		if (c < 32) continue;


		int index = c - 32;
		int col = index % 16;
		int row = index / 16;

		float srcX = col * srcSize;
		float srcY = row * srcSize;

	
		Sprite_Draw(g_FontTexId, currentX, y, drawW, drawH, srcX, srcY, srcSize, srcSize);

		currentX += drawW; // 光标后移
	}
}

void Inventory_Initialize()
{
	// 1. 初始化数据库
	DefineItems();


	g_Inventory.resize(MAX_SLOTS);
	for (auto& slot : g_Inventory) {
		slot.itemId = -1;
		slot.count = 0;
	}


	g_TexBackground = Texture_LoadFromFile(L"resource/texture/ui_inventory_bg.png");
	if (g_TexBackground == -1) g_TexBackground = Texture_LoadFromFile(L"resource/texture/white.png");
	g_TexCursor = Texture_LoadFromFile(L"resource/texture/ui_cursor.png");
	if (g_TexCursor == -1) g_TexCursor = Texture_LoadFromFile(L"resource/texture/white.png");
	g_TexIcons = Texture_LoadFromFile(L"resource/texture/ui_icons.png");
	g_FontTexId = Texture_LoadFromFile(L"resource/texture/consolab_ascii_512.png");
	
	Inventory_AddItem(0, 5); // 5瓶血药
	Inventory_AddItem(1, 1); // 1把剑
	Inventory_AddItem(3, 1);
	Inventory_AddItem(2, 4);
}

void Inventory_Update(double elapsed_time)
{
	if (g_ItemUseCooldown > 0.0f) {
		g_ItemUseCooldown -= (float)elapsed_time;
	}
	if (KeyLogger_IsTrigger(KK_I)) {
		g_IsOpen = !g_IsOpen;
	}

	if (!g_IsOpen) return;


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

	if (g_ItemUseCooldown > 0.0f) return;
	// 3. 使用道具 (按回车或空格)
	if (KeyLogger_IsTrigger(KK_ENTER)) {
		InventorySlot& slot = g_Inventory[g_CursorIndex];
		if (!slot.isEmpty()) {
			int itemId = slot.itemId;
			bool isUsed = false; // 标记是否成功使用了道具

			
			if (itemId == 0) 
			{
				//Player_Heal(50.0f);
				isUsed = true;
			}
			else if (itemId == 2) 
			{
				//Player_Heal(20.0f);
				isUsed = true;
			}
			else if (g_ItemDatabase[slot.itemId].type == ItemType::Consumable)
			{
				
				//printf("Used generic consumable\n");
				isUsed = true;
			}

			// --- 统一处理扣除和冷却 ---
			if (isUsed)
			{
				Inventory_RemoveItem(g_CursorIndex, 1);
				g_ItemUseCooldown = 0.2f;
			}
		}
	}
}

void Inventory_Draw()
{
	if (!g_IsOpen) return;

	
	Direct3D_SetBlendState(true);
	Direct3D_SetDepthEnable(false); 

	// --- 1. 计算面板位置 ---
	float screenW = (float)Direct3D_GetBackBufferWidth();
	float screenH = (float)Direct3D_GetBackBufferHeight();

	float panelW = 400.0f;
	float panelH = 400.0f;
	float panelX = (screenW - panelW) / 2.0f;
	float panelY = (screenH - panelH) / 2.0f;

	// 绘制大背景面板 (半透明灰色)
	Sprite_Draw(g_TexBackground, panelX, panelY, panelW, panelH, { 0.2f, 0.2f, 0.2f, 0.9f });

	// --- 2. 遍历绘制所有格子 ---
	float slotSize = 50.0f;
	float gap = 10.0f;
	float startX = panelX + 30.0f;
	float startY = panelY + 30.0f;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		// 计算当前格子的屏幕坐标
		int col = i % COLS;
		int row = i / COLS;
		float x = startX + col * (slotSize + gap);
		float y = startY + row * (slotSize + gap);

		
		XMFLOAT4 bgColor = (i == g_CursorIndex) ? XMFLOAT4{ 0.3f, 0.3f, 0.3f, 0.8f } : XMFLOAT4{ 0.0f, 0.0f, 0.0f, 0.5f };
		Sprite_Draw(g_TexBackground, x, y, slotSize, slotSize, bgColor);

		
		InventorySlot& slot = g_Inventory[i];
		if (!slot.isEmpty()) {
			ItemDefinition& def = g_ItemDatabase[slot.itemId];

			// 计算 UV 裁剪
			float iconRawSize = 32.0f;
			float srcX = (def.uvIndex * iconRawSize);
			float srcY = 0.0f;

			// 绘制图标
			if (g_TexIcons != -1) {
				Sprite_Draw(g_TexIcons, x + 4, y + 4, slotSize - 8, slotSize - 8, srcX, srcY, iconRawSize, iconRawSize);
			}

			// 绘制数量 (大于1才显示)
			if (slot.count > 1) {
				DrawDebugText(x + 2, y + slotSize - 16, "%d", slot.count);
			}
		}

		if (i == g_CursorIndex) {
			// 画一个比格子稍微大一点的框
			Sprite_Draw(g_TexCursor, x - 4, y - 4, slotSize + 8, slotSize + 8);

		
			if (!slot.isEmpty()) {
				ItemDefinition& def = g_ItemDatabase[slot.itemId];

				
				std::string nameStr(def.name.begin(), def.name.end());
				DrawDebugText(panelX + 20, panelY + panelH - 60, "Name: %s", nameStr.c_str());

			
				std::string descStr(def.desc.begin(), def.desc.end());
				DrawDebugText(panelX + 20, panelY + panelH - 30, "Desc: %s", descStr.c_str());
			}
			else {
				DrawDebugText(panelX + 20, panelY + panelH - 40, "Empty Slot");
			}
		}
	}
}

void Inventory_Finalize()
{
	g_Inventory.clear();
	g_ItemDatabase.clear();
}



bool Inventory_AddItem(int itemId, int count)
{
	
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

	
	for (auto& slot : g_Inventory) {
		if (slot.isEmpty()) {
			slot.itemId = itemId;
			slot.count = count;
			return true;
		}
	}

	return false; 
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

int Inventory_GetItemUVIndex(int itemId)
{
	if (g_ItemDatabase.find(itemId) == g_ItemDatabase.end()) return 0;

	return g_ItemDatabase[itemId].uvIndex;
}

int Inventory_GetIconsTextureID()
{
	return g_TexIcons;
}
