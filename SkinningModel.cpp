#include "SkinningModel.h"
#include "direct3d.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "WICTextureLoader11.h"
#include <filesystem>
#include "SkinningShader.h"
#include <assimp/cimport.h>

using namespace DirectX;

// --- 辅助函数与矩阵转换（底层逻辑不变） ---
static XMMATRIX AiToXMMatrix(const aiMatrix4x4& m) {
	return XMMatrixTranspose(XMMATRIX(
		m.a1, m.a2, m.a3, m.a4,
		m.b1, m.b2, m.b3, m.b4,
		m.c1, m.c2, m.c3, m.c4,
		m.d1, m.d2, m.d3, m.d4
	));
}

SkinningModel::~SkinningModel() {
	Release();
}

// --- 核心加载逻辑 ---

bool SkinningModel::Load(const std::string& fileName, float scale) {
	// 【修改点】：用 aiImportFile 替代 Assimp::Importer
	const aiScene* scene = aiImportFile(fileName.c_str(),
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_LimitBoneWeights |
		aiProcess_JoinIdenticalVertices);

	if (!scene || !scene->mRootNode) return false;

	ProcessSkeleton(scene);
	ProcessMesh(scene, scale);
	ProcessAnimation(scene, "Default", scale);
	ProcessMaterials(scene, fileName);

	// 【修改点】：记得手动释放内存
	aiReleaseImport(scene);
	return true;
}

// [核心追加方法]：从另一个文件（如 Run.fbx）提取动画
bool SkinningModel::LoadAnimation(const std::string& animName, const std::string& fileName, float scale) {
	// 【修改点】：用 aiImportFile 替代 Assimp::Importer
	const aiScene* scene = aiImportFile(fileName.c_str(), aiProcess_ConvertToLeftHanded);

	if (!scene || scene->mNumAnimations == 0) {
		if (scene) aiReleaseImport(scene);
		return false;
	}

	ProcessAnimation(scene, animName, scale);

	// 【修改点】：释放内存
	aiReleaseImport(scene);
	return true;
}

// --- 动画处理解耦（关键修改点） ---

void SkinningModel::ProcessAnimation(const aiScene* scene, const std::string& animName, float scale) {
	if (scene->mNumAnimations == 0) return;

	// 取出第一个动画（Mixamo 文件通常只有一个）
	aiAnimation* anim = scene->mAnimations[0];

	// 创建局部 Animation 对象
	Animation newAnim;
	newAnim.duration = (float)anim->mDuration;
	newAnim.ticksPerSecond = (float)(anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0);

	for (uint32_t i = 0; i < anim->mNumChannels; ++i) {
		aiNodeAnim* ch = anim->mChannels[i];
		std::string nodeName = ch->mNodeName.C_Str();
		AnimationChannel& dst = newAnim.channels[nodeName];

		// 采样位移
		if (ch->mPositionKeys != nullptr && ch->mNumPositionKeys > 0) {
			for (uint32_t k = 0; k < ch->mNumPositionKeys; ++k)
				dst.positions.push_back({ (float)ch->mPositionKeys[k].mTime,
					{ ch->mPositionKeys[k].mValue.x * scale,
					  ch->mPositionKeys[k].mValue.y * scale,
					  ch->mPositionKeys[k].mValue.z * scale } });
		}

		// 采样旋转 (之前崩溃的地方，加上双重保护)
		if (ch->mRotationKeys != nullptr && ch->mNumRotationKeys > 0) {
			for (uint32_t k = 0; k < ch->mNumRotationKeys; ++k)
				dst.rotations.push_back({ (float)ch->mRotationKeys[k].mTime,
					{ ch->mRotationKeys[k].mValue.x, ch->mRotationKeys[k].mValue.y,
					  ch->mRotationKeys[k].mValue.z, ch->mRotationKeys[k].mValue.w } });
		}

		// 采样缩放
		if (ch->mScalingKeys != nullptr && ch->mNumScalingKeys > 0) {
			for (uint32_t k = 0; k < ch->mNumScalingKeys; ++k)
				dst.scales.push_back({ (float)ch->mScalingKeys[k].mTime,
					{ ch->mScalingKeys[k].mValue.x, ch->mScalingKeys[k].mValue.y, ch->mScalingKeys[k].mValue.z } });
		}
	}

	// 存入动画库
	mAnimations[animName] = std::move(newAnim);
}

// --- 访问接口实现 ---

const Animation* SkinningModel::GetAnimation(const std::string& name) const {
	auto it = mAnimations.find(name);
	if (it != mAnimations.end()) {
		return &(it->second);
	}
	return nullptr;
}

const Animation* SkinningModel::GetDefaultAnimation() const {
	if (mAnimations.empty()) return nullptr;
	// 优先返回名为 "Default" 的，如果没有则返回 map 里的第一个
	auto it = mAnimations.find("Default");
	if (it != mAnimations.end()) return &(it->second);
	return &(mAnimations.begin()->second);
}

// --- 底层逻辑保持不变的部分 ---

void SkinningModel::ProcessSkeleton(const aiScene* scene) {
	for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
		aiMesh* mesh = scene->mMeshes[i];
		for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
			aiBone* aibone = mesh->mBones[b];
			std::string name = aibone->mName.C_Str();
			if (mSkeleton.nameToIndex.find(name) == mSkeleton.nameToIndex.end()) {
				Bone newBone;
				newBone.name = name;
				newBone.invBindPose = AiToXMMatrix(aibone->mOffsetMatrix);
				newBone.bindPose = XMMatrixInverse(nullptr, newBone.invBindPose);
				int index = (int)mSkeleton.bones.size();
				mSkeleton.bones.push_back(newBone);
				mSkeleton.nameToIndex[name] = index;
			}
		}
	}
	auto FindParent = [&](auto self, aiNode* node, int parentIdx) -> void {
		std::string name = node->mName.C_Str();
		int currentIdx = parentIdx;
		if (mSkeleton.nameToIndex.count(name)) {
			currentIdx = mSkeleton.nameToIndex[name];
			mSkeleton.bones[currentIdx].parent = parentIdx;
		}
		for (uint32_t i = 0; i < node->mNumChildren; ++i) self(self, node->mChildren[i], currentIdx);
		};
	FindParent(FindParent, scene->mRootNode, -1);
}

void SkinningModel::ProcessMesh(const aiScene* scene, float scale) {
	for (uint32_t m = 0; m < scene->mNumMeshes; ++m) {
		aiMesh* mesh = scene->mMeshes[m];
		std::vector<VertexSkinning> vertices(mesh->mNumVertices);
		std::vector<uint32_t> indices;
		SkinningMesh newMesh;
		newMesh.MaterialIndex = mesh->mMaterialIndex;

		for (uint32_t v = 0; v < mesh->mNumVertices; ++v) {
			vertices[v].Position = { mesh->mVertices[v].x * scale, mesh->mVertices[v].y * scale, mesh->mVertices[v].z * scale };
			vertices[v].Normal = { mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z };
			if (mesh->HasTextureCoords(0)) vertices[v].TexCoord = { mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
			vertices[v].Color = { 1, 1, 1, 1 };
		}
		for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
			aiBone* aibone = mesh->mBones[b];
			int boneIdx = mSkeleton.nameToIndex[aibone->mName.C_Str()];
			for (uint32_t w = 0; w < aibone->mNumWeights; ++w) {
				vertices[aibone->mWeights[w].mVertexId].AddBoneData(boneIdx, aibone->mWeights[w].mWeight);
			}
		}
		for (auto& v : vertices) {
			float total = v.BoneWeights[0] + v.BoneWeights[1] + v.BoneWeights[2] + v.BoneWeights[3];
			if (total > 0.0f) for (int i = 0; i < 4; i++) v.BoneWeights[i] /= total;
		}
		for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
			indices.push_back(mesh->mFaces[f].mIndices[0]);
			indices.push_back(mesh->mFaces[f].mIndices[1]);
			indices.push_back(mesh->mFaces[f].mIndices[2]);
		}
		newMesh.IndexCount = (uint32_t)indices.size();
		D3D11_BUFFER_DESC vbd = { sizeof(VertexSkinning) * (UINT)vertices.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
		D3D11_SUBRESOURCE_DATA vsd = { vertices.data(), 0, 0 };
		Direct3D_GetDevice()->CreateBuffer(&vbd, &vsd, &newMesh.VertexBuffer);
		D3D11_BUFFER_DESC ibd = { sizeof(uint32_t) * (UINT)indices.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
		D3D11_SUBRESOURCE_DATA isd = { indices.data(), 0, 0 };
		Direct3D_GetDevice()->CreateBuffer(&ibd, &isd, &newMesh.IndexBuffer);
		mMeshes.push_back(newMesh);
	}
}

void SkinningModel::ProcessMaterials(const struct aiScene* scene, const std::string& fileName) {
	mMaterials.clear();
	mMaterials.resize(scene->mNumMaterials);
	for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
		const aiMaterial* aimat = scene->mMaterials[i];
		SkinningMaterial& mat = mMaterials[i];
		aiString texPath;
		if (aimat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
			if (auto atex = scene->GetEmbeddedTexture(texPath.C_Str()))
				mat.DiffuseSRV = CreateSRVFromEmbeddedTexture(atex);
		}
		aiColor3D color(1.f, 1.f, 1.f);
		if (aimat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
			mat.DiffuseColor = XMFLOAT4(color.r, color.g, color.b, 1.0f);
	}
}

ID3D11ShaderResourceView* SkinningModel::CreateSRVFromEmbeddedTexture(const aiTexture* tex) {
	if (!tex) return nullptr;
	ID3D11Device* device = Direct3D_GetDevice();
	ID3D11DeviceContext* context = Direct3D_GetDeviceContext();
	ID3D11ShaderResourceView* srv = nullptr;
	if (tex->mHeight == 0) {
		DirectX::CreateWICTextureFromMemory(device, context, reinterpret_cast<const uint8_t*>(tex->pcData), tex->mWidth, nullptr, &srv);
		return srv;
	}
	return nullptr; // 原始数据情况略
}

void SkinningModel::Release() {
	for (auto& m : mMeshes) {
		if (m.VertexBuffer) m.VertexBuffer->Release();
		if (m.IndexBuffer) m.IndexBuffer->Release();
	}
	mMeshes.clear();
	for (auto& m : mMaterials) {
		if (m.DiffuseSRV) m.DiffuseSRV->Release();
	}
	mMaterials.clear();
}

void SkinningModel::Draw() {
	ID3D11DeviceContext* ctx = Direct3D_GetDeviceContext();
	for (const auto& mesh : mMeshes) {
		if (mesh.MaterialIndex < mMaterials.size()) {
			const SkinningMaterial& mat = mMaterials[mesh.MaterialIndex];
			XMFLOAT4 finalColor = mat.DiffuseColor;
			if (finalColor.x == 0.0f && finalColor.y == 0.0f && finalColor.z == 0.0f) finalColor = { 1,1,1,1 };
			SkinningShader_3D_SetMaterialColor(finalColor);
			ctx->PSSetShaderResources(0, 1, &mat.DiffuseSRV);
		}
		UINT stride = sizeof(VertexSkinning), offset = 0;
		ctx->IASetVertexBuffers(0, 1, &mesh.VertexBuffer, &stride, &offset);
		ctx->IASetIndexBuffer(mesh.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx->DrawIndexed(mesh.IndexCount, 0, 0);
	}
}