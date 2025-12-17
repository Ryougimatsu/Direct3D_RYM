#pragma once

#include <DirectXMath.h>

// 初始化自由相机
void DebugCamera_Initialize(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rot);

// 更新自由相机 (处理输入: WASD移动, 鼠标旋转)
void DebugCamera_Update(double elapsed_time);

// 获取矩阵 (供渲染使用)
DirectX::XMFLOAT4X4 DebugCamera_GetViewMatrix();
DirectX::XMFLOAT4X4 DebugCamera_GetProjectionMatrix();

// 获取当前位置 (方便你把相机位置打印出来，或者传给光照)
DirectX::XMFLOAT3 DebugCamera_GetPosition();

void DebugCamera_SetPosition(DirectX::XMFLOAT3 pos);