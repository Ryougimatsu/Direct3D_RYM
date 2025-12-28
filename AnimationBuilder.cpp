// AnimationBuilder.cpp
#include "AnimationBuilder.h"
#include <DirectXMath.h>
using namespace DirectX;

static DirectX::XMFLOAT3 AiToXMFLOAT3(const aiVector3D& v)
{
	return XMFLOAT3((float)v.x, (float)v.y, (float)v.z);
}

static DirectX::XMFLOAT4 AiToXMFLOAT4(const aiQuaternion& q)
{
	// Assimp quaternion: (x, y, z, w)
	return XMFLOAT4((float)q.x, (float)q.y, (float)q.z, (float)q.w);
}

Animation BuildAnimationFromAssimp(const aiScene* scene, int animIndex)
{
	Animation anim;

	if (!scene || scene->mNumAnimations == 0) return anim;
	if (animIndex < 0 || animIndex >= (int)scene->mNumAnimations) animIndex = 0;

	const aiAnimation* a = scene->mAnimations[animIndex];

	anim.duration = a->mDuration;
	anim.ticksPerSecond = (a->mTicksPerSecond != 0.0) ? a->mTicksPerSecond : 25.0;

	// Mixamo 动画一般每个骨骼一个 aiNodeAnim
	for (unsigned int i = 0; i < a->mNumChannels; ++i)
	{
		const aiNodeAnim* ch = a->mChannels[i];
		std::string boneName = ch->mNodeName.C_Str();

		AnimationChannel& dst = anim.channels[boneName];

		// position keys
		dst.positions.reserve(ch->mNumPositionKeys);
		for (unsigned int j = 0; j < ch->mNumPositionKeys; ++j)
		{
			const auto& key = ch->mPositionKeys[j];
			double t = key.mTime;          // in ticks
			XMFLOAT3 v = AiToXMFLOAT3(key.mValue);
			dst.positions.emplace_back(t, v);
		}

		// rotation keys
		dst.rotations.reserve(ch->mNumRotationKeys);
		for (unsigned int j = 0; j < ch->mNumRotationKeys; ++j)
		{
			const auto& key = ch->mRotationKeys[j];
			double t = key.mTime;
			XMFLOAT4 q = AiToXMFLOAT4(key.mValue);
			dst.rotations.emplace_back(t, q);
		}

		// scale keys
		dst.scales.reserve(ch->mNumScalingKeys);
		for (unsigned int j = 0; j < ch->mNumScalingKeys; ++j)
		{
			const auto& key = ch->mScalingKeys[j];
			double t = key.mTime;
			XMFLOAT3 s = AiToXMFLOAT3(key.mValue);
			dst.scales.emplace_back(t, s);
		}
	}

	return anim;
}
