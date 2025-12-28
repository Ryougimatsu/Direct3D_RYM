#pragma once
#include <vector>
#include <string>
#include <map>
#include "Skeleton.h"
#include "Animation.h"
#include <DirectXMath.h>

class Animator
{
public:
	Animator();

	// 播放接口：fadeTime 为过渡时长（秒）
	void PlayAnimation(const Animation* pNewAnim, bool loop = true, float fadeTime = 0.3f);

	// 更新进度
	void Update(double deltaTime);

	// 生成最终矩阵数组（主入口）
	std::vector<DirectX::XMMATRIX> GetFinalBoneMatrices(const Skeleton& skeleton);

private:
	// 当前动画状态
	const Animation* mCurrentAnim;
	double mCurrentTime;
	bool   mLoop;

	// 过渡状态（用于平滑切换动作）
	const Animation* mOldAnim;
	double mOldTime;
	float  mFadeTime;   // 过渡总时长
	float  mFadeFactor; // 0.0=旧, 1.0=新

	/**
	 * @brief 核心计算函数：负责采样、混合以及递归计算骨骼的全局矩阵
	 * @param boneIndex 当前计算的骨骼索引
	 * @param skeleton 骨骼层级数据
	 * @param parentMatrix 父骨骼的全局动画矩阵
	 * @return 当前骨骼的模型空间全局动画矩阵
	 */
	DirectX::XMMATRIX CalculateBone(
		int boneIndex,
		const Skeleton& skeleton,
		const DirectX::XMMATRIX& parentMatrix
	);

	// 关键帧采样与插值内部函数
	DirectX::XMVECTOR SamplePosition(const AnimationChannel& ch, double t);
	DirectX::XMVECTOR SampleRotation(const AnimationChannel& ch, double t);
	DirectX::XMVECTOR SampleScale(const AnimationChannel& ch, double t);

	// 查找关键帧索引的模板函数
	template<typename T>
	int FindKeyIndex(const std::vector<std::pair<double, T>>& keys, double t);
};