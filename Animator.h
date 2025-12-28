#pragma once
#include <vector>
#include <string>
#include "Skeleton.h"
#include "Animation.h"
#include <DirectXMath.h>

class Animator
{
public:
	Animator();

	void PlayAnimation(const Animation* anim, bool loop = true);
	void Update(double deltaTime);

	// 生成最终矩阵数组（骨骼顺序与 skeleton.bones 对应）
	std::vector<DirectX::XMMATRIX> GetFinalBoneMatrices(const Skeleton& skeleton);

private:
	const Animation* mCurrentAnim;
	double mCurrentTime;
	bool mLoop;

	// 递归计算每根骨骼
	DirectX::XMMATRIX CalculateBone(
		int boneIndex,
		const Skeleton& skeleton,
		const DirectX::XMMATRIX& parent
	);

	// 关键帧插值
	DirectX::XMVECTOR SamplePosition(const AnimationChannel& ch, double t);
	DirectX::XMVECTOR SampleRotation(const AnimationChannel& ch, double t);
	DirectX::XMVECTOR SampleScale(const AnimationChannel& ch, double t);

	// 找到左右关键帧
	template<typename T>
	int FindKeyIndex(const std::vector<std::pair<double, T>>& keys, double t);
};
