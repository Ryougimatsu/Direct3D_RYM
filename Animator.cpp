#include "Animator.h"
#include "Skeleton.h"
#include <windows.h>
#include <algorithm> // 用于 std::min

using namespace DirectX;

Animator::Animator()
{
	mCurrentAnim = nullptr;
	mCurrentTime = 0.0;
	mLoop = true;

	// 新增过渡变量初始化
	mOldAnim = nullptr;
	mOldTime = 0.0;
	mFadeTime = 0.3f;
	mFadeFactor = 1.0f;
}

// ------------------------------------------------------------------
// 播放接口：支持平滑过渡
// ------------------------------------------------------------------
void Animator::PlayAnimation(const Animation* anim, bool loop, float fadeTime)
{
	if (mCurrentAnim == anim) return;

	// 如果当前有动画在播放，则将其切换为“旧动画”开始淡出
	if (mCurrentAnim != nullptr && fadeTime > 0.0f)
	{
		mOldAnim = mCurrentAnim;
		mOldTime = mCurrentTime;
		mFadeFactor = 0.0f; // 从 0 开始混合
		mFadeTime = fadeTime;
	}
	else
	{
		mOldAnim = nullptr;
		mFadeFactor = 1.0f; // 无需过渡，直接显示新动画
	}

	mCurrentAnim = anim;
	mCurrentTime = 0.0;
	mLoop = loop;
}

void Animator::Update(double deltaTime)
{
	if (!mCurrentAnim) return;
	double scaledDelta = deltaTime * m_SpeedScale;
	// 1. 更新当前动画时间
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

	// 2. 更新旧动画时间及过渡因子
	if (mOldAnim)
	{
		double oldTps = (mOldAnim->ticksPerSecond != 0.0) ? mOldAnim->ticksPerSecond : 25.0;
		mOldTime += scaledDelta * oldTps;
		mOldTime = fmod(mOldTime, mOldAnim->duration); // 旧动画通常保持循环播放直到消失

		mFadeFactor += (float)deltaTime / mFadeTime;
		if (mFadeFactor >= 1.0f)
		{
			mFadeFactor = 1.0f;
			mOldAnim = nullptr; // 过渡结束，彻底移除旧动作引用
		}
	}
}

// ------------------------------------------------------------------
// 核心逻辑：采样并混合两个动画的姿态
// ------------------------------------------------------------------
XMMATRIX Animator::CalculateBone(int boneIndex, const Skeleton& skeleton, const XMMATRIX& parentMatrix)
{
	const Bone& bone = skeleton.bones[boneIndex];

	// --- 1. 获取初始 Local Bind Pose (逻辑保持不变) ---
	XMMATRIX localBind = bone.bindPose;
	if (bone.parent >= 0)
	{
		XMMATRIX invParentBind = XMMatrixInverse(nullptr, skeleton.bones[bone.parent].bindPose);
		localBind = XMMatrixMultiply(bone.bindPose, invParentBind);
	}

	// 分解出初始的 S, R, T 作为默认值
	XMVECTOR S, R, T;
	XMMatrixDecompose(&S, &R, &T, localBind);

	// --- 2. 采样新动画的 T, R, S ---
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

	// --- 3. 采样旧动画并执行插值混合 (核心新增) ---
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

		// 执行插值：混合 旧动作 -> 新动作
		T = XMVectorLerp(oldT, curT, mFadeFactor);
		S = XMVectorLerp(oldS, curS, mFadeFactor);
		// 旋转必须使用球形线性插值 (Slerp)
		R = XMQuaternionSlerp(oldR, curR, mFadeFactor);
		R = XMQuaternionNormalize(R);
	}
	else
	{
		// 无需混合，直接使用当前动画值
		T = curT;
		R = curR;
		S = curS;
	}

	// --- 4. 组合并计算全局矩阵 (逻辑保持不变) ---
	XMMATRIX local = XMMatrixScalingFromVector(S) * XMMatrixRotationQuaternion(R) * XMMatrixTranslationFromVector(T);

	return XMMatrixMultiply(local, parentMatrix);
}

// ------------------------------------------------------------------
// 关键帧查找与采样 (保持原有底层逻辑不变)
// ------------------------------------------------------------------
template<typename T>
int Animator::FindKeyIndex(const std::vector<std::pair<double, T>>& keys, double t)
{
	if (keys.empty()) return -1;
	if (t <= keys.front().first) return 0;
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
	if (j >= (int)ch.positions.size()) return XMLoadFloat3(&ch.positions[i].second);
	auto& k1 = ch.positions[i]; auto& k2 = ch.positions[j];
	float alpha = (k2.first - k1.first > 0.0) ? float((t - k1.first) / (k2.first - k1.first)) : 0.0f;
	return XMVectorLerp(XMLoadFloat3(&k1.second), XMLoadFloat3(&k2.second), alpha);
}

XMVECTOR Animator::SampleScale(const AnimationChannel& ch, double t)
{
	if (ch.scales.empty()) return XMVectorSet(1, 1, 1, 0);
	int i = FindKeyIndex(ch.scales, t);
	int j = i + 1;
	if (j >= (int)ch.scales.size()) return XMLoadFloat3(&ch.scales[i].second);
	auto& k1 = ch.scales[i]; auto& k2 = ch.scales[j];
	float alpha = (k2.first - k1.first > 0.0) ? float((t - k1.first) / (k2.first - k1.first)) : 0.0f;
	return XMVectorLerp(XMLoadFloat3(&k1.second), XMLoadFloat3(&k2.second), alpha);
}


DirectX::XMMATRIX Animator::GetBoneGlobalMatrix(int boneIndex) const
{
	if (boneIndex < 0 || boneIndex >= m_CachedGlobalMatrices.size())
		return DirectX::XMMatrixIdentity();
	return m_CachedGlobalMatrices[boneIndex];
}

XMVECTOR Animator::SampleRotation(const AnimationChannel& ch, double t)
{
	if (ch.rotations.empty()) return XMQuaternionIdentity();
	int i = FindKeyIndex(ch.rotations, t);
	int j = i + 1;
	if (j >= (int)ch.rotations.size()) return XMLoadFloat4(&ch.rotations[i].second);
	auto& k1 = ch.rotations[i]; auto& k2 = ch.rotations[j];
	float alpha = (k2.first - k1.first > 0.0) ? float((t - k1.first) / (k2.first - k1.first)) : 0.0f;
	XMVECTOR q = XMQuaternionSlerp(XMLoadFloat4(&k1.second), XMLoadFloat4(&k2.second), alpha);
	return XMQuaternionNormalize(q);
}

// ------------------------------------------------------------------
// 生成最终矩阵数组 (保持原有层级逻辑不变)
// ------------------------------------------------------------------
std::vector<XMMATRIX> Animator::GetFinalBoneMatrices(const Skeleton& skeleton)
{
	size_t count = skeleton.bones.size();

	// 确保缓存大小一致
	if (m_CachedGlobalMatrices.size() != count)
		m_CachedGlobalMatrices.resize(count);

	std::vector<XMMATRIX> finalMatrices(count);

	for (int i = 0; i < (int)count; i++)
	{
		const Bone& bone = skeleton.bones[i];
		XMMATRIX parentMat = (bone.parent == -1)
			? XMMatrixIdentity()
			: m_CachedGlobalMatrices[bone.parent];  // 父骨骼的 global

		// 计算当前骨骼的 global
		XMMATRIX global = CalculateBone(i, skeleton, parentMat);

		// 更新缓存的全局矩阵，给挂点用
		m_CachedGlobalMatrices[i] = global;

		// 蒙皮矩阵：invBindPose * global（你现在的顺序就保持不变即可）
		finalMatrices[i] = XMMatrixMultiply(bone.invBindPose, global);
		// 如果你原来是 global * invBindPose，就用 XMMatrixMultiply(global, bone.invBindPose);
	}

	return finalMatrices;
}