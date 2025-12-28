#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <DirectXMath.h>

// 一个骨骼在动画里的单独轨道
struct AnimationChannel
{
	// (时间, 值)
	std::vector<std::pair<double, DirectX::XMFLOAT3>> positions;
	std::vector<std::pair<double, DirectX::XMFLOAT4>> rotations;
	std::vector<std::pair<double, DirectX::XMFLOAT3>> scales;
};

// 整个动画
struct Animation
{
	double duration = 0.0;        // 动画时长（tick）
	double ticksPerSecond = 25.0; // 速率

	// boneName -> AnimationChannel
	std::unordered_map<std::string, AnimationChannel> channels;
};
