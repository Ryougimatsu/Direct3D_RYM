#include "SkinningModel.h"
#include "direct3d.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "WICTextureLoader11.h"
#include <filesystem>
using namespace DirectX;


std::wstring ToWString(const std::string& str) {
	if (str.empty()) return L"";
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

// 矩阵转换助手：Assimp -> DirectX (列主序处理)
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

bool SkinningModel::Load(const std::string& fileName, float scale) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_LimitBoneWeights | // 限制每个顶点最多4根骨骼
		aiProcess_JoinIdenticalVertices);

	if (!scene || !scene->mRootNode) return false;

	ProcessSkeleton(scene);   // 1. 提取骨骼层级与偏移矩阵
	ProcessMesh(scene, scale); // 2. 提取顶点、索引与权重
	ProcessAnimation(scene,scale);  // 3. 提取动画轨道
	ProcessMaterials(scene, fileName);
	return true;
}

void SkinningModel::ProcessSkeleton(const aiScene* scene) {
	// 步骤 A: 收集所有在 Mesh 中真正被使用的骨骼名字和 OffsetMatrix
	for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
		aiMesh* mesh = scene->mMeshes[i];
		for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
			aiBone* aibone = mesh->mBones[b];
			std::string name = aibone->mName.C_Str();

			if (mSkeleton.nameToIndex.find(name) == mSkeleton.nameToIndex.end()) {
				Bone newBone;
				newBone.name = name;

				// 1. 提取 Inverse Bind Pose (Offset Matrix)
				newBone.invBindPose = AiToXMMatrix(aibone->mOffsetMatrix);
				newBone.bindPose = DirectX::XMMatrixInverse(nullptr, newBone.invBindPose);

				int index = (int)mSkeleton.bones.size();
				mSkeleton.bones.push_back(newBone);
				mSkeleton.nameToIndex[name] = index;
			}
		}
	}

	// 步骤 B: 遍历节点树建立 Parent 关系
	auto FindParent = [&](auto self, aiNode* node, int parentIdx) -> void {
		std::string name = node->mName.C_Str();
		int currentIdx = parentIdx;

		if (mSkeleton.nameToIndex.count(name)) {
			currentIdx = mSkeleton.nameToIndex[name];
			mSkeleton.bones[currentIdx].parent = parentIdx;
		}

		for (uint32_t i = 0; i < node->mNumChildren; ++i) {
			self(self, node->mChildren[i], currentIdx);
		}
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

		// 加载顶点基础数据
		for (uint32_t v = 0; v < mesh->mNumVertices; ++v) {
			vertices[v].Position = { mesh->mVertices[v].x * scale, mesh->mVertices[v].y * scale, mesh->mVertices[v].z * scale };
			vertices[v].Normal = { mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z };
			if (mesh->HasTextureCoords(0))
				vertices[v].TexCoord = { mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
			vertices[v].Color = { 1, 1, 1, 1 };
		}

		// 核心：处理权重
		for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
			aiBone* aibone = mesh->mBones[b];
			int boneIdx = mSkeleton.nameToIndex[aibone->mName.C_Str()];

			for (uint32_t w = 0; w < aibone->mNumWeights; ++w) {
				uint32_t vID = aibone->mWeights[w].mVertexId;
				float weight = aibone->mWeights[w].mWeight;
				vertices[vID].AddBoneData(boneIdx, weight);
			}
		}

		// 权重归一化处理
		for (auto& v : vertices) {
			float total = v.BoneWeights[0] + v.BoneWeights[1] + v.BoneWeights[2] + v.BoneWeights[3];
			if (total > 0.0f) {
				for (int i = 0; i < 4; i++) v.BoneWeights[i] /= total;
			}
		}

		// 索引处理
		for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
			indices.push_back(mesh->mFaces[f].mIndices[0]);
			indices.push_back(mesh->mFaces[f].mIndices[1]);
			indices.push_back(mesh->mFaces[f].mIndices[2]);
		}

		// 创建 D3D 缓冲
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

void SkinningModel::ProcessAnimation(const aiScene* scene, float scale) {
	if (scene->mNumAnimations == 0) return;
	aiAnimation* anim = scene->mAnimations[0]; // 默认取第一个
	mAnimation.duration = anim->mDuration;
	mAnimation.ticksPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0;

	for (uint32_t i = 0; i < anim->mNumChannels; ++i) {
		aiNodeAnim* ch = anim->mChannels[i];
		std::string name = ch->mNodeName.C_Str();
		AnimationChannel& dst = mAnimation.channels[name];

		for (uint32_t k = 0; k < ch->mNumPositionKeys; ++k)
			dst.positions.push_back({ ch->mPositionKeys[k].mTime,
				{ ch->mPositionKeys[k].mValue.x * scale,
				  ch->mPositionKeys[k].mValue.y * scale,
				  ch->mPositionKeys[k].mValue.z * scale } });
		for (uint32_t k = 0; k < ch->mNumRotationKeys; ++k)
			dst.rotations.push_back({ ch->mRotationKeys[k].mTime, {ch->mRotationKeys[k].mValue.x, ch->mRotationKeys[k].mValue.y, ch->mRotationKeys[k].mValue.z, ch->mRotationKeys[k].mValue.w} });
		for (uint32_t k = 0; k < ch->mNumScalingKeys; ++k) {
			dst.scales.push_back({ ch->mScalingKeys[k].mTime,
				{ ch->mScalingKeys[k].mValue.x, ch->mScalingKeys[k].mValue.y, ch->mScalingKeys[k].mValue.z } });
		}
	}
}

void SkinningModel::ProcessMaterials(const struct aiScene* scene, const std::string& fileName)
{
	mMaterials.clear();
	mMaterials.resize(scene->mNumMaterials);

	for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial* aimat = scene->mMaterials[i];
		SkinningMaterial& mat = mMaterials[i];

		// 获取 Diffuse 贴图路径/索引
		aiString texPath;
		if (aimat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
		{
			// 检查是否为内嵌贴图索引（格式通常为 "*0", "*1" 等）
			if (auto atex = scene->GetEmbeddedTexture(texPath.C_Str()))
			{
				mat.DiffuseSRV = CreateSRVFromEmbeddedTexture(atex);
			}
			else
			{
				// 容错：如果不是内嵌贴图，再走外部文件加载逻辑（代码略）
			}
		}

		// 获取材质默认颜色
		aiColor3D color(1.f, 1.f, 1.f);
		if (aimat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
			mat.DiffuseColor = XMFLOAT4(color.r, color.g, color.b, 1.0f);
	}
}

ID3D11ShaderResourceView* SkinningModel::CreateSRVFromEmbeddedTexture(const aiTexture* tex)
{
	if (!tex) return nullptr;

	ID3D11Device* device = Direct3D_GetDevice();
	ID3D11DeviceContext* context = Direct3D_GetDeviceContext();
	ID3D11ShaderResourceView* srv = nullptr;

	// 情况 1：压缩图像（PNG/JPG/TGA）
	// mHeight == 0 表示 pcData 存储的是压缩后的二进制流，mWidth 是该流的字节长度
	if (tex->mHeight == 0)
	{
		HRESULT hr = DirectX::CreateWICTextureFromMemory(
			device,
			context, // 传入 Context 极其重要！它会自动生成 Mipmaps，解决闪烁和模糊
			reinterpret_cast<const uint8_t*>(tex->pcData),
			tex->mWidth,
			nullptr,
			&srv
		);
		return SUCCEEDED(hr) ? srv : nullptr;
	}

	// 情况 2：未压缩的原始数据 (RGBA8888)
	// 这种情况在 Mixamo FBX 中较少见，但为了健壮性需要保留
	else
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = tex->mWidth;
		desc.Height = tex->mHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Assimp 默认通常是 BGRA
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = tex->pcData;
		initData.SysMemPitch = tex->mWidth * 4;

		ID3D11Texture2D* pTex2D = nullptr;
		if (SUCCEEDED(device->CreateTexture2D(&desc, &initData, &pTex2D)))
		{
			device->CreateShaderResourceView(pTex2D, nullptr, &srv);
			pTex2D->Release();
		}
		return srv;
	}
}

void SkinningModel::Release() {
	for (auto& m : mMeshes) {
		if (m.VertexBuffer) m.VertexBuffer->Release();
		if (m.IndexBuffer) m.IndexBuffer->Release();
	}
	mMeshes.clear();
}