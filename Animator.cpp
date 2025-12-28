#include "Animator.h"
#include "skeleton.h"      // ★ 新增：包含 Skeleton / Bone 定义
#include <windows.h>
using namespace DirectX;

Animator::Animator()
{
	mCurrentAnim = nullptr;
	mCurrentTime = 0.0;
	mLoop = true;
}

void Animator::PlayAnimation(const Animation* anim, bool loop)
{
	mCurrentAnim = anim;
	mCurrentTime = 0.0;
	mLoop = loop;
}

void Animator::Update(double deltaTime)
{
	if (!mCurrentAnim)
	{
		OutputDebugStringA("[Animator] No current animation\n");
		return;
	}

	double tps = (mCurrentAnim->ticksPerSecond != 0.0)
		? mCurrentAnim->ticksPerSecond
		: 25.0;

	double prevTime = mCurrentTime;

	mCurrentTime += deltaTime * tps;

	if (mLoop)
	{
		while (mCurrentTime >= mCurrentAnim->duration)
			mCurrentTime -= mCurrentAnim->duration;
	}
	else
	{
		if (mCurrentTime > mCurrentAnim->duration)
			mCurrentTime = mCurrentAnim->duration;
	}

}

// ------------------------------------------------------------------
// 关键帧查找（保留原来的模板实现）
// ------------------------------------------------------------------
template<typename T>
int Animator::FindKeyIndex(const std::vector<std::pair<double, T>>& keys, double t)
{
	if (keys.empty()) return -1;
	if (t <= keys.front().first) return 0;

	for (int i = 0; i < (int)keys.size() - 1; i++)
	{
		if (t < keys[i + 1].first)
			return i;
	}

	return (int)keys.size() - 1;
}

XMVECTOR Animator::SamplePosition(const AnimationChannel& ch, double t)
{
	if (ch.positions.empty()) return XMVectorZero();

	int i = FindKeyIndex(ch.positions, t);
	int j = i + 1;

	if (j >= (int)ch.positions.size())
		return XMLoadFloat3(&ch.positions[i].second);

	auto& k1 = ch.positions[i];
	auto& k2 = ch.positions[j];

	double span = k2.first - k1.first;
	float alpha = (span > 0.0) ? float((t - k1.first) / span) : 0.0f;

	XMVECTOR p1 = XMLoadFloat3(&k1.second);
	XMVECTOR p2 = XMLoadFloat3(&k2.second);

	return XMVectorLerp(p1, p2, alpha);
}

XMVECTOR Animator::SampleScale(const AnimationChannel& ch, double t)
{
	if (ch.scales.empty()) return XMVectorSet(1, 1, 1, 0);

	int i = FindKeyIndex(ch.scales, t);
	int j = i + 1;

	if (j >= (int)ch.scales.size())
		return XMLoadFloat3(&ch.scales[i].second);

	auto& k1 = ch.scales[i];
	auto& k2 = ch.scales[j];

	double span = k2.first - k1.first;
	float alpha = (span > 0.0) ? float((t - k1.first) / span) : 0.0f;

	XMVECTOR s1 = XMLoadFloat3(&k1.second);
	XMVECTOR s2 = XMLoadFloat3(&k2.second);

	return XMVectorLerp(s1, s2, alpha);
}

XMVECTOR Animator::SampleRotation(const AnimationChannel& ch, double t)
{
	if (ch.rotations.empty()) return XMQuaternionIdentity();

	int i = FindKeyIndex(ch.rotations, t);
	int j = i + 1;

	if (j >= (int)ch.rotations.size())
		return XMLoadFloat4(&ch.rotations[i].second);

	auto& k1 = ch.rotations[i];
	auto& k2 = ch.rotations[j];

	double span = k2.first - k1.first;
	float alpha = (span > 0.0) ? float((t - k1.first) / span) : 0.0f;

	XMVECTOR q1 = XMLoadFloat4(&k1.second);
	XMVECTOR q2 = XMLoadFloat4(&k2.second);

	XMVECTOR q = XMQuaternionSlerp(q1, q2, alpha);

	return XMQuaternionNormalize(q);
}

// ------------------------------------------------------------------
// 计算单个骨骼的 *global* 动画矩阵
// ------------------------------------------------------------------
XMMATRIX Animator::CalculateBone(int boneIndex, const Skeleton& skeleton, const XMMATRIX& parentMatrix)
{
	const Bone& bone = skeleton.bones[boneIndex];
	XMVECTOR T, R, S;

	// 1) 计算 Local Bind Pose (相对于父骨骼的初始姿态)
	XMMATRIX localBind = bone.bindPose;
	if (bone.parent >= 0)
	{
		const Bone& parentBone = skeleton.bones[bone.parent];
		XMMATRIX invParentBind = XMMatrixInverse(nullptr, parentBone.bindPose);

		// ★ 修正 1：行主序下 Local = Global * inv(Parent)
		// 逻辑：当前骨骼的全局绑定姿态 乘以 父骨骼全局绑定姿态的逆
		localBind = XMMatrixMultiply(bone.bindPose, invParentBind);
	}

	// 2) 分解出初始的 S, R, T
	XMMatrixDecompose(&S, &R, &T, localBind);

	// 3) 动画采样 (逻辑保持不变)
	if (mCurrentAnim) {
		auto it = mCurrentAnim->channels.find(bone.name);
		static int sFrameCount = 0;

		if (it != mCurrentAnim->channels.end()) {
			const AnimationChannel& ch = it->second;
			if (!ch.positions.empty()) T = SamplePosition(ch, mCurrentTime);
			if (!ch.rotations.empty()) R = SampleRotation(ch, mCurrentTime);
			if (!ch.scales.empty())    S = SampleScale(ch, mCurrentTime);
		}
	}

	// 4) 组合当前的 Local 矩阵 (缩放 * 旋转 * 平移)
	XMMATRIX local = XMMatrixScalingFromVector(S) * XMMatrixRotationQuaternion(R) * XMMatrixTranslationFromVector(T);

	// ★ 修正 2：行主序下 Global = Local * ParentGlobal
	// 逻辑：先在局部空间变换，再叠加父级的全局变换
	return XMMatrixMultiply(local, parentMatrix);
}



std::vector<DirectX::XMMATRIX> Animator::GetFinalBoneMatrices(const Skeleton& skeleton)
{
	size_t count = skeleton.bones.size();
	std::vector<XMMATRIX> finalMatrices(count);


	std::vector<XMMATRIX> globalMatrices(count);

	for (int i = 0; i < (int)count; i++)
	{
		const Bone& bone = skeleton.bones[i];

		// 从 globalMatrices 获取父级矩阵，而不是从结果数组取
		XMMATRIX parentMat = (bone.parent == -1) ? XMMatrixIdentity() : globalMatrices[bone.parent];

		// 1. 计算当前骨骼在模型空间下的全局动画姿态 (Local * Parent)
		globalMatrices[i] = CalculateBone(i, skeleton, parentMat);

		// 2. 合成蒙皮矩阵 (Final = invBind * Global)
		// 这一步的结果仅用于发送给 GPU，不参与层级累加
		finalMatrices[i] = XMMatrixMultiply(bone.invBindPose, globalMatrices[i]);
	}

	return finalMatrices;
}
