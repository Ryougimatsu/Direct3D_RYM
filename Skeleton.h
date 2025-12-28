#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <DirectXMath.h>

struct Bone
{
	std::string name;

	int parent; // parent index (-1 if root)

	// --- bind pose (rest pose) ---
	DirectX::XMMATRIX bindPose;        // global bind pose matrix
	DirectX::XMMATRIX invBindPose;     // inverse bind pose
};

struct Skeleton
{
	std::vector<Bone> bones;
	std::unordered_map<std::string, int> nameToIndex;
};
