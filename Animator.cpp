#include "Animator.h"
#include "Skeleton.h"
#include <windows.h>
#include <algorithm> 
#include <cmath>     

using namespace DirectX;

// ======================================================================================
// 1. 构造与初始化
// ======================================================================================
Animator::Animator()
{
	// 当前动画状态
	mCurrentAnim = nullptr;
	mCurrentTime = 0.0;
	mLoop = true;

	// 过渡状态
	mOldAnim = nullptr;
	mOldTime = 0.0;
	mFadeTime = 0.3f;
	mFadeFactor = 1.0f;

	// 播放速度
	m_SpeedScale = 1.0f;
}

// ======================================================================================
// 2. 动画控制 (Control)
// ======================================================================================

void Animator::PlayAnimation(const Animation* anim, bool loop, float fadeTime)
{
	if (mCurrentAnim == anim) return;

	// 如果当前有动画在播放，且需要过渡，则将其切换为“旧动画”开始淡出
	if (mCurrentAnim != nullptr && fadeTime > 0.0f)
	{
		mOldAnim = mCurrentAnim;
		mOldTime = mCurrentTime;
		mFadeFactor = 0.0f; // 0.0 代表完全是旧动画，随时间向 1.0 (新动画) 过渡
		mFadeTime = fadeTime;
	}
	else
	{
		mOldAnim = nullptr;
		mFadeFactor = 1.0f; // 1.0 代表直接显示新动画，无过渡
	}

	// 设置新动画
	mCurrentAnim = anim;
	mCurrentTime = 0.0;
	mLoop = loop;
}

void Animator::Update(double deltaTime)
{
	if (!mCurrentAnim) return;

	// 应用播放速度
	double scaledDelta = deltaTime * m_SpeedScale;

	// --- A. 更新当前动画时间 ---
	double tps = (mCurrentAnim->ticksPerSecond != 0.0) ? mCurrentAnim->ticksPerSecond : 25.0;
	mCurrentTime += scaledDelta * tps;

	if (mLoop)
	{
		mCurrentTime = fmod(mCurrentTime, mCurrentAnim->duration);
	}
	else
	{
		mCurrentTime = (std::min)(mCurrentTime, mCurrentAnim->duration);
	}

	// --- B. 更新旧动画时间及过渡因子 ---
	if (mOldAnim)
	{
		double oldTps = (mOldAnim->ticksPerSecond != 0.0) ? mOldAnim->ticksPerSecond : 25.0;
		mOldTime += scaledDelta * oldTps;

		// 旧动画通常保持循环播放直到彻底淡出，防止定格在最后一帧看起来不自然
		mOldTime = fmod(mOldTime, mOldAnim->duration);

		// 更新混合权重
		mFadeFactor += (float)deltaTime / mFadeTime;
		if (mFadeFactor >= 1.0f)
		{
			mFadeFactor = 1.0f;
			mOldAnim = nullptr; // 过渡结束，彻底移除旧动作引用
		}
	}
}

// ======================================================================================
// 3. 数据获取 (Data Access)
// ======================================================================================

std::vector<XMMATRIX> Animator::GetFinalBoneMatrices(const Skeleton& skeleton)
{
	size_t count = skeleton.bones.size();

	// 确保缓存大小与骨骼数量一致
	if (m_CachedGlobalMatrices.size() != count)
		m_CachedGlobalMatrices.resize(count);

	std::vector<XMMATRIX> finalMatrices(count);

	for (int i = 0; i < (int)count; i++)
	{
		const Bone& bone = skeleton.bones[i];

		// 获取父骨骼的全局矩阵
		XMMATRIX parentMat = (bone.parent == -1)
			? XMMatrixIdentity()
			: m_CachedGlobalMatrices[bone.parent];

		// 计算当前骨骼的全局矩阵 (包含动画采样与混合)
		XMMATRIX global = CalculateBone(i, skeleton, parentMat);

		// 1. 更新缓存 (用于子骨骼计算或外部挂点)
		m_CachedGlobalMatrices[i] = global;

		// 2. 计算蒙皮矩阵 (用于 Shader)
		// 公式：Final = InvBindPose * GlobalTransform
		finalMatrices[i] = XMMatrixMultiply(bone.invBindPose, global);
	}

	return finalMatrices;
}

DirectX::XMMATRIX Animator::GetBoneGlobalMatrix(int boneIndex) const
{
	if (boneIndex < 0 || boneIndex >= m_CachedGlobalMatrices.size())
		return DirectX::XMMatrixIdentity();
	return m_CachedGlobalMatrices[boneIndex];
}

// ======================================================================================
// 4. 核心计算逻辑 (Core Calculation)
// ======================================================================================

XMMATRIX Animator::CalculateBone(int boneIndex, const Skeleton& skeleton, const XMMATRIX& parentMatrix)
{
	const Bone& bone = skeleton.bones[boneIndex];

	// --- A. 获取初始 Local Bind Pose ---
	// 如果没有动画数据，这是默认姿态
	XMMATRIX localBind = bone.bindPose;
	if (bone.parent >= 0)
	{
		// 这一步是为了从 BindPose 中提取出相对于父骨骼的 Local 变换
		XMMATRIX invParentBind = XMMatrixInverse(nullptr, skeleton.bones[bone.parent].bindPose);
		localBind = XMMatrixMultiply(bone.bindPose, invParentBind);
	}

	// 分解出初始的 S, R, T
	XMVECTOR S, R, T;
	XMMatrixDecompose(&S, &R, &T, localBind);

	// --- B. 采样新动画 (Current) ---
	XMVECTOR curT = T, curR = R, curS = S;
	if (mCurrentAnim)
	{
		auto it = mCurrentAnim->channels.find(bone.name);
		if (it != mCurrentAnim->channels.end())
		{
			const AnimationChannel& ch = it->second;
			if (!ch.positions.empty()) curT = SamplePosition(ch, mCurrentTime);
			if (!ch.rotations.empty()) curR = SampleRotation(ch, mCurrentTime);
			if (!ch.scales.empty())    curS = SampleScale(ch, mCurrentTime);
		}
	}

	// --- C. 采样旧动画并混合 (Old -> Current) ---
	if (mOldAnim && mFadeFactor < 1.0f)
	{
		XMVECTOR oldT = T, oldR = R, oldS = S;
		auto it = mOldAnim->channels.find(bone.name);
		if (it != mOldAnim->channels.end())
		{
			const AnimationChannel& ch = it->second;
			if (!ch.positions.empty()) oldT = SamplePosition(ch, mOldTime);
			if (!ch.rotations.empty()) oldR = SampleRotation(ch, mOldTime);
			if (!ch.scales.empty())    oldS = SampleScale(ch, mOldTime);
		}

		// 线性插值混合位置和缩放
		T = XMVectorLerp(oldT, curT, mFadeFactor);
		S = XMVectorLerp(oldS, curS, mFadeFactor);

		// 球形插值混合旋转 (Slerp)
		R = XMQuaternionSlerp(oldR, curR, mFadeFactor);
		R = XMQuaternionNormalize(R);
	}
	else
	{
		// 无需混合，直接使用当前值
		T = curT;
		R = curR;
		S = curS;
	}

	// --- D. 组合矩阵 ---
	XMMATRIX local = XMMatrixScalingFromVector(S) * XMMatrixRotationQuaternion(R) * XMMatrixTranslationFromVector(T);

	return XMMatrixMultiply(local, parentMatrix);
}

// ======================================================================================
// 5. 辅助函数：插值与采样 (Helpers)
// ======================================================================================

template<typename T>
int Animator::FindKeyIndex(const std::vector<std::pair<double, T>>& keys, double t)
{
	if (keys.empty()) return -1;
	if (t <= keys.front().first) return 0;

	// 简单的线性查找，如果关键帧很多可以优化为二分查找
	for (int i = 0; i < (int)keys.size() - 1; i++)
	{
		if (t < keys[i + 1].first) return i;
	}
	return (int)keys.size() - 1;
}

XMVECTOR Animator::SamplePosition(const AnimationChannel& ch, double t)
{
	if (ch.positions.empty()) return XMVectorZero();

	int i = FindKeyIndex(ch.positions, t);
	int j = i + 1;

	// 边界检查：如果是最后一帧，直接返回该帧数据
	if (j >= (int)ch.positions.size()) return XMLoadFloat3(&ch.positions[i].second);

	auto& k1 = ch.positions[i];
	auto& k2 = ch.positions[j];

	float alpha = (k2.first - k1.first > 0.0) ? float((t - k1.first) / (k2.first - k1.first)) : 0.0f;
	return XMVectorLerp(XMLoadFloat3(&k1.second), XMLoadFloat3(&k2.second), alpha);
}

XMVECTOR Animator::SampleRotation(const AnimationChannel& ch, double t)
{
	if (ch.rotations.empty()) return XMQuaternionIdentity();

	int i = FindKeyIndex(ch.rotations, t);
	int j = i + 1;

	if (j >= (int)ch.rotations.size()) return XMLoadFloat4(&ch.rotations[i].second);

	auto& k1 = ch.rotations[i];
	auto& k2 = ch.rotations[j];

	float alpha = (k2.first - k1.first > 0.0) ? float((t - k1.first) / (k2.first - k1.first)) : 0.0f;

	XMVECTOR q = XMQuaternionSlerp(XMLoadFloat4(&k1.second), XMLoadFloat4(&k2.second), alpha);
	return XMQuaternionNormalize(q);
}

XMVECTOR Animator::SampleScale(const AnimationChannel& ch, double t)
{
	if (ch.scales.empty()) return XMVectorSet(1, 1, 1, 0);

	int i = FindKeyIndex(ch.scales, t);
	int j = i + 1;

	if (j >= (int)ch.scales.size()) return XMLoadFloat3(&ch.scales[i].second);

	auto& k1 = ch.scales[i];
	auto& k2 = ch.scales[j];

	float alpha = (k2.first - k1.first > 0.0) ? float((t - k1.first) / (k2.first - k1.first)) : 0.0f;
	return XMVectorLerp(XMLoadFloat3(&k1.second), XMLoadFloat3(&k2.second), alpha);
}