#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <d3d11.h>
#include <DirectXMath.h>
#include <map>
#include "Skeleton.h"   //
#include "Animation.h"  //

// 对应 SkinningShader_VS.hlsl 的输入布局
struct VertexSkinning
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT4 Color;
	DirectX::XMFLOAT2 TexCoord;
	uint32_t BoneIndices[4]; //
	float BoneWeights[4];    //

	VertexSkinning() {
		memset(this, 0, sizeof(VertexSkinning));
	}

	// 辅助函数：添加骨骼权重并确保归一化
	void AddBoneData(uint32_t boneID, float weight) {
		for (int i = 0; i < 4; i++) {
			if (BoneWeights[i] == 0.0f) {
				BoneIndices[i] = boneID;
				BoneWeights[i] = weight;
				return;
			}
		}
	}
};

struct SkinningMesh {
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	uint32_t IndexCount = 0;
	uint32_t MaterialIndex = 0;
};

struct SkinningMaterial
{
	ID3D11ShaderResourceView* DiffuseSRV = nullptr;
	DirectX::XMFLOAT4 DiffuseColor = { 1,1,1,1 };
};

class SkinningModel {
public:
	SkinningModel() = default;
	~SkinningModel();

	// 加载接口
	bool Load(const std::string& fileName, float scale = 1.0f);
	bool LoadAnimation(const std::string& animName, const std::string& fileName, float scale = 1.0f);
	void Release();
	void Draw();
	// 数据访问
	const Skeleton& GetSkeleton() const { return mSkeleton; }
	const Animation* GetAnimation(const std::string& name) const;
	const Animation* GetDefaultAnimation() const;
	const std::vector<SkinningMesh>& GetMeshes() const { return mMeshes; }
	const std::vector<SkinningMaterial>& GetMaterials() const { return mMaterials; }

private:
	Skeleton mSkeleton;
	std::map<std::string, Animation> mAnimations;
	std::vector<SkinningMesh> mMeshes;
	std::vector<SkinningMaterial> mMaterials;
	std::unordered_map<std::string, ID3D11ShaderResourceView*> mTextures;

	// 内部加载助手
	void ProcessSkeleton(const struct aiScene* scene);
	void ProcessMesh(const struct aiScene* scene, float scale);
	void ProcessAnimation(const struct aiScene* scene, const std::string& animName, float scale);
	void ProcessMaterials(const struct aiScene* scene, const std::string& fileName);
	ID3D11ShaderResourceView* CreateSRVFromEmbeddedTexture(const struct aiTexture* tex);
};