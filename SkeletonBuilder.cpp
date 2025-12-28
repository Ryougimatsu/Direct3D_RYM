#include "SkeletonBuilder.h"
#include <unordered_set>

using namespace DirectX;

//---------------------------------------------------------------------------
// Assimp 矩阵 -> XMMATRIX (行主序转换)
//---------------------------------------------------------------------------
XMMATRIX AiToXMMATRIX(const aiMatrix4x4& m)
{
	// Assimp 的平移在第4列，转置后变为 D3D 行主序要求的第4行
	return XMMatrixTranspose(XMMATRIX(
		(float)m.a1, (float)m.a2, (float)m.a3, (float)m.a4,
		(float)m.b1, (float)m.b2, (float)m.b3, (float)m.b4,
		(float)m.c1, (float)m.c2, (float)m.c3, (float)m.c4,
		(float)m.d1, (float)m.d2, (float)m.d3, (float)m.d4));
}

// 前向声明
static void BuildSkeletonRecursive(
	aiNode* node,
	const XMMATRIX& parentGlobal,
	int parentBoneIndex,
	Skeleton& skeleton);

//---------------------------------------------------------------------------
// 主入口：构建 Skeleton
//---------------------------------------------------------------------------
Skeleton BuildSkeletonFromAssimp(const aiScene* scene)
{
	Skeleton skel;
	if (!scene) return skel;

	// 第 1 & 2 步：收集骨骼并提取 Assimp 权威的 invBindPose (mOffsetMatrix)
	for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
	{
		aiMesh* mesh = scene->mMeshes[m];
		for (unsigned int i = 0; i < mesh->mNumBones; ++i)
		{
			aiBone* aibone = mesh->mBones[i];
			std::string name = aibone->mName.C_Str();

			if (skel.nameToIndex.find(name) == skel.nameToIndex.end())
			{
				Bone b;
				b.name = name;
				b.parent = -1;
				// ★ 关键：直接使用 Assimp 提供的 OffsetMatrix 
				b.invBindPose = AiToXMMATRIX(aibone->mOffsetMatrix);
				b.bindPose = XMMatrixIdentity();

				int index = (int)skel.bones.size();
				skel.bones.push_back(b);
				skel.nameToIndex[name] = index;
			}
		}
	}

	// 第 3 步：递归构建层级关系
	XMMATRIX identity = XMMatrixIdentity();
	BuildSkeletonRecursive(scene->mRootNode, identity, -1, skel);

	return skel;
}

//---------------------------------------------------------------------------
// 递归函数：计算 bindPose 并建立父子关系
//---------------------------------------------------------------------------
static void BuildSkeletonRecursive(
	aiNode* node,
	const XMMATRIX& parentGlobal,
	int parentBoneIndex,
	Skeleton& skeleton)
{
	if (!node) return;

	std::string nodeName = node->mName.C_Str();

	// 当前节点的 local 变换
	XMMATRIX local = AiToXMMATRIX(node->mTransformation);
	// ★ 统一行主序：Local * ParentGlobal
	XMMATRIX global = local * parentGlobal;

	int thisBoneIndex = parentBoneIndex;

	auto it = skeleton.nameToIndex.find(nodeName);
	if (it != skeleton.nameToIndex.end())
	{
		thisBoneIndex = it->second;
		Bone& b = skeleton.bones[thisBoneIndex];

		b.parent = parentBoneIndex;
		b.bindPose = global; // 记录当前的全局绑定姿态
	}

	// 递归子节点
	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		BuildSkeletonRecursive(node->mChildren[i], global, thisBoneIndex, skeleton);
	}
}