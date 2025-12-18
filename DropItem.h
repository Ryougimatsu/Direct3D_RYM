#pragma once
#include <DirectXMath.h>

// 初始化掉落系统
void DropItem_Initialize();

// 清理资源
void DropItem_Finalize();

// 更新掉落物 (处理旋转动画、检测玩家拾取)
void DropItem_Update(double elapsed_time);

// 绘制掉落物
void DropItem_Draw();

// [核心接口] 在指定位置生成一个掉落物 (参数: 位置, 道具ID)
void DropItem_Spawn(DirectX::XMFLOAT3 position, int itemId);