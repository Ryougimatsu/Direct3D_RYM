#include <assert.h>
#include "direct3d.h"
#include "texture.h"
#include "model.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "WICTextureLoader11.h"
#include "shader_3d.h"
#include "shader3d_unlit.h"
#include "Shader_Shadow.h"
struct Vertex3D
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT3 normal;   // 法線ベクトル
	XMFLOAT4 color;
	XMFLOAT2 uv; // uv座標
	uint32_t boneIndices[4]; // 影响该顶点的4个骨骼ID
	XMFLOAT4 boneWeights;    // 对应的4个权重
};
namespace {
	int g_TextureWhite = -1;

	void BindDiffuseTexture(MODEL* model, aiMaterial* material)
	{
		aiString texturePath;
		if (material &&
			material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS &&
			texturePath.length != 0)
		{
			auto it = model->Texture.find(texturePath.C_Str());
			if (it != model->Texture.end() && it->second)
			{
				Direct3D_GetDeviceContext()->PSSetShaderResources(0, 1, &it->second);
				return;
			}
		}

		Texture_Set(g_TextureWhite);
	}
}

MODEL* ModelLoad(const char* FileName, float size)
{
	if (!FileName || !Direct3D_GetDevice())
		return nullptr;

	MODEL* model = new MODEL{};

	model->AiScene = aiImportFile(
		FileName,
		aiProcessPreset_TargetRealtime_MaxQuality |
		aiProcess_ConvertToLeftHanded
	);
	if (!model->AiScene)
	{
		delete model;
		return nullptr;
	}

	model->VertexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes]{};
	model->IndexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes]{};

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// -----------------------------
		// 顶点缓冲
		// -----------------------------
		{
			Vertex3D* vertex = new Vertex3D[mesh->mNumVertices];

			bool hasUV0 = mesh->HasTextureCoords(0) && mesh->mTextureCoords[0] != nullptr;
			bool hasNormals = mesh->HasNormals();

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				// 位置（左手系）
				vertex[v].position = XMFLOAT3(
					mesh->mVertices[v].x * size,
					mesh->mVertices[v].y * size,
					mesh->mVertices[v].z * size
				);

				// UV：有就用，没有就给 (0,0)
				if (hasUV0)
				{
					vertex[v].uv = XMFLOAT2(
						mesh->mTextureCoords[0][v].x,
						mesh->mTextureCoords[0][v].y
					);
				}
				else
				{
					vertex[v].uv = XMFLOAT2(0.0f, 0.0f);
				}

				// 顶点色：先全白
				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

				// 法线：有就用，没有就给默认向上
				if (hasNormals)
				{
					vertex[v].normal = XMFLOAT3(
						mesh->mNormals[v].x,
						mesh->mNormals[v].y,
						mesh->mNormals[v].z
					);
				}
				else
				{
					vertex[v].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
				}

				// 骨骼索引/权重(静态模型先清0，免得有脏数据)
				vertex[v].boneIndices[0] = 0;
				vertex[v].boneIndices[1] = 0;
				vertex[v].boneIndices[2] = 0;
				vertex[v].boneIndices[3] = 0;
				vertex[v].boneWeights = XMFLOAT4(0, 0, 0, 0);
			}

			D3D11_BUFFER_DESC bd{};
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(Vertex3D) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sd{};
			sd.pSysMem = vertex;

			HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

			delete[] vertex;
			if (FAILED(hr))
			{
				ModelRelease(model);
				return nullptr;
			}
		}

		// -----------------------------
		// 索引缓冲
		// -----------------------------
		{
			unsigned int indexCount = mesh->mNumFaces * 3;
			unsigned int* index = new unsigned int[indexCount];

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];
				assert(face->mNumIndices == 3);

				index[f * 3 + 0] = face->mIndices[0];
				index[f * 3 + 1] = face->mIndices[1];
				index[f * 3 + 2] = face->mIndices[2];
			}

			D3D11_BUFFER_DESC bd{};
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * indexCount;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sd{};
			sd.pSysMem = index;

			HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);

			delete[] index;
			if (FAILED(hr))
			{
				ModelRelease(model);
				return nullptr;
			}
		}
	}

	// 备用白色纹理
	if (g_TextureWhite < 0)
		g_TextureWhite = Texture_LoadFromFile(L"resource/texture/white.png");

	// ---------- 嵌入纹理 ----------
	for (unsigned int i = 0; i < model->AiScene->mNumTextures; i++)
	{
		aiTexture* aitexture = model->AiScene->mTextures[i];

		ID3D11ShaderResourceView* texture = nullptr;
		ID3D11Resource* resource = nullptr;

		CreateWICTextureFromMemory(
			Direct3D_GetDevice(),
			Direct3D_GetDeviceContext(),
			(const uint8_t*)aitexture->pcData,
			(size_t)aitexture->mWidth,
			&resource,
			&texture);

		if (texture)
		{
			if (resource) resource->Release();
			model->Texture[aitexture->mFilename.C_Str()] = texture;
		}
		else if (resource)
		{
			resource->Release();
		}
	}

	// ---------- 外部纹理 ----------
	const std::string modelPath(FileName);
	size_t pos = modelPath.find_last_of("/\\");
	std::string directory = (pos != std::string::npos) ? modelPath.substr(0, pos) : "";

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiString filename;
		aiMaterial* aiMaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];

		if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &filename) != AI_SUCCESS)
			continue;

		if (filename.length == 0)
			continue;

		if (model->Texture.count(filename.C_Str()))
			continue;

		std::string texfilename = directory + "/" + filename.C_Str();

		int len = MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, nullptr, 0);
		wchar_t* pWideFilename = new wchar_t[len];
		MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, pWideFilename, len);

		ID3D11ShaderResourceView* texture = nullptr;
		ID3D11Resource* resource = nullptr;

		HRESULT hr = CreateWICTextureFromFile(
			Direct3D_GetDevice(),
			Direct3D_GetDeviceContext(),
			pWideFilename,
			&resource,
			&texture);

		delete[] pWideFilename;

		if (SUCCEEDED(hr) && texture)
		{
			if (resource) resource->Release();
			model->Texture[filename.C_Str()] = texture;
		}
		else
		{
			if (resource) resource->Release();
			// 找不到贴图就用白色占位
			model->Texture[filename.C_Str()] = nullptr;
		}
	}

	return model;
}


MODEL* ModelLoadS(const char* FileName, float size)
{
	if (!FileName || !Direct3D_GetDevice())
		return nullptr;

	MODEL* model = new MODEL{};

	model->AiScene = aiImportFile(
		FileName,
		aiProcessPreset_TargetRealtime_MaxQuality |
		aiProcess_ConvertToLeftHanded
	);
	if (!model->AiScene)
	{
		delete model;
		return nullptr;
	}

	model->VertexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes]{};
	model->IndexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes]{};

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// 顶点缓冲
		{
			Vertex3D* vertex = new Vertex3D[mesh->mNumVertices];

			bool hasUV0 = mesh->HasTextureCoords(0) && mesh->mTextureCoords[0] != nullptr;
			bool hasNormals = mesh->HasNormals();

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				// 注意：你这里用的是另一套坐标系
				vertex[v].position = XMFLOAT3(
					mesh->mVertices[v].x * size,
					-mesh->mVertices[v].z * size,
					mesh->mVertices[v].y * size
				);

				if (hasUV0)
				{
					vertex[v].uv = XMFLOAT2(
						mesh->mTextureCoords[0][v].x,
						mesh->mTextureCoords[0][v].y);
				}
				else
				{
					vertex[v].uv = XMFLOAT2(0.0f, 0.0f);
				}

				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

				if (hasNormals)
				{
					vertex[v].normal = XMFLOAT3(
						mesh->mNormals[v].x,
						-mesh->mNormals[v].z,
						mesh->mNormals[v].y);
				}
				else
				{
					vertex[v].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
				}

				vertex[v].boneIndices[0] = 0;
				vertex[v].boneIndices[1] = 0;
				vertex[v].boneIndices[2] = 0;
				vertex[v].boneIndices[3] = 0;
				vertex[v].boneWeights = XMFLOAT4(0, 0, 0, 0);
			}

			D3D11_BUFFER_DESC bd{};
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(Vertex3D) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sd{};
			sd.pSysMem = vertex;

			HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

			delete[] vertex;
			if (FAILED(hr))
			{
				ModelRelease(model);
				return nullptr;
			}
		}

		// 索引缓冲（和前面一样）
		{
			unsigned int indexCount = mesh->mNumFaces * 3;
			unsigned int* index = new unsigned int[indexCount];

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];
				assert(face->mNumIndices == 3);

				index[f * 3 + 0] = face->mIndices[0];
				index[f * 3 + 1] = face->mIndices[1];
				index[f * 3 + 2] = face->mIndices[2];
			}

			D3D11_BUFFER_DESC bd{};
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * indexCount;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sd{};
			sd.pSysMem = index;

			HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);

			delete[] index;
			if (FAILED(hr))
			{
				ModelRelease(model);
				return nullptr;
			}
		}
	}



	if (g_TextureWhite < 0)
		g_TextureWhite = Texture_LoadFromFile(L"resource/texture/white.png");



	// テクスチャ読み込み (Load texture)
	for (unsigned int i = 0; i < model->AiScene->mNumTextures; i++)
	{
		aiTexture* aitexture = model->AiScene->mTextures[i];

		ID3D11ShaderResourceView* texture = nullptr;
		ID3D11Resource* resource = nullptr;


		CreateWICTextureFromMemory(
			Direct3D_GetDevice(),
			Direct3D_GetDeviceContext(),
			(const uint8_t*)aitexture->pcData,
			(size_t)aitexture->mWidth,
			&resource, // release!!!!
			&texture);

		if (texture)
		{
			if (resource) resource->Release();
			model->Texture[aitexture->mFilename.data] = texture;
		}
		else if (resource)
		{
			resource->Release();
		}
	}


	const std::string modelPath(FileName);

	// 最後の '/' または '\\' の位置を探す (Windows対応)
	size_t pos = modelPath.find_last_of("/\\");
	std::string directory;

	if (pos != std::string::npos) {
		directory = modelPath.substr(0, pos); // ファイル名を除いた部分
	}
	else {
		directory = ""; // パスに区切りがない場合 (ファイル名のみ)
	}

	// テクスチャがFBXとは別に用意されている場合
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiString filename;
		aiMaterial* aiMaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &filename);

		if (filename.length == 0) {
			continue;
		}

		if (model->Texture.count(filename.C_Str())) {
			continue;
		}

		ID3D11ShaderResourceView* texture = nullptr;
		ID3D11Resource* resource = nullptr;

		std::string texfilename = directory + "/" + filename.C_Str();

		int len = MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, nullptr, 0);
		wchar_t* pWideFilename = new wchar_t[len];
		MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, pWideFilename, len);

		HRESULT hr = CreateWICTextureFromFile(
			Direct3D_GetDevice(),
			Direct3D_GetDeviceContext(),
			pWideFilename,
			&resource,
			&texture);

		delete[] pWideFilename;

		if (SUCCEEDED(hr) && texture)
		{
			if (resource) resource->Release();
			model->Texture[filename.C_Str()] = texture;
		}
		else
		{
			if (resource) resource->Release();
			model->Texture[filename.C_Str()] = nullptr;
		}
	}


	return model;
}





void ModelRelease(MODEL* model)
{
	if (!model)
		return;

	const unsigned int meshCount = model->AiScene ? model->AiScene->mNumMeshes : 0;
	for (unsigned int m = 0; m < meshCount; m++)
	{
		if (model->VertexBuffer && model->VertexBuffer[m]) model->VertexBuffer[m]->Release();
		if (model->IndexBuffer && model->IndexBuffer[m]) model->IndexBuffer[m]->Release();
	}

	delete[] model->VertexBuffer;
	delete[] model->IndexBuffer;

	// --- 修改开始 ---
	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : model->Texture)
	{
		// 必须检查指针是否为空！
		if (pair.second != nullptr)
		{
			pair.second->Release();
		}
	}
	// --- 修改结束 ---

	if (model->AiScene) aiReleaseImport(model->AiScene);

	delete model;
}

void ModelDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
	if (!model || !model->AiScene)
		return;

	// シェーダーを描画パイプラインに設定
	Shader_3D_Begin();

	// プリミティブトポロジ設定
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Shader_3D_SetWorldMatrix(mtxWorld);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) {

		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		BindDiffuseTexture(model, aimaterial);
		aiColor3D diffuse(1.0f, 1.0f, 1.0f);
		aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		Shader_3D_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });
		// 頂点バッファを描画パイプラインに設定
		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
		Direct3D_GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		// ポリゴン描画命令発行
		Direct3D_GetDeviceContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3,0,0);
	}
}

void ModelUnlitDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
	if (!model || !model->AiScene)
		return;

	// シェーダーを描画パイプラインに設定
	Shader3DUnilt_Begin();

	// プリミティブトポロジ設定
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Shader3DUnilt_SetWorldMatrix(mtxWorld);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) {

		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		BindDiffuseTexture(model, aimaterial);
		aiColor3D diffuse(1.0f, 1.0f, 1.0f);
		aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		Shader3DUnilt_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });
		// 頂点バッファを描画パイプラインに設定
		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
		Direct3D_GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		// ポリゴン描画命令発行
		Direct3D_GetDeviceContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0);
	}
}

void ModelWeaponDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
	if (!model || !model->AiScene)
		return;

	Shader_3D_Begin();
	Shader_3D_SetWorldMatrix(mtxWorld);

	ID3D11DeviceContext* context = Direct3D_GetDeviceContext();

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) {
		aiMesh* mesh = model->AiScene->mMeshes[m];
		aiMaterial* pMaterial = model->AiScene->mMaterials[mesh->mMaterialIndex];

		aiString texPath;
		// 绑定颜色贴图到 t0
		BindDiffuseTexture(model, pMaterial);

		// 绑定法线贴图到 t1 (如果你的 Shader 支持)
		ID3D11ShaderResourceView* normalSRV = nullptr;
		if (pMaterial->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
			auto it = model->Texture.find(texPath.C_Str());
			if (it != model->Texture.end()) normalSRV = it->second;
		}
		context->PSSetShaderResources(1, 1, &normalSRV);

		// 设置顶点和索引缓冲
		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
		context->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		// 绘制
		context->DrawIndexed(mesh->mNumFaces * 3, 0, 0);
	}
}

void ModelDrawShadow(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
	if (!model || !model->AiScene)
		return;

	// 1. 设置矩阵
	Shader_Shadow_SetWorldMatrix(mtxWorld);

	// 2. 只需要设置拓扑，不需要 Shader_Begin
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 3. 遍历网格
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) {
		// 不需要设置材质和贴图！Pass 1 不需要颜色！

		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
		Direct3D_GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		Direct3D_GetDeviceContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0);
	}
}



AABB ModelGetAABB(MODEL* model, const DirectX::XMFLOAT3& position)
{
	if (!model || !model->AiScene || model->AiScene->mNumMeshes == 0)
		return { position, position };

	AABB aabb;
	aiVector3D min = model->AiScene->mMeshes[0]->mAABB.mMin;
	aiVector3D max = model->AiScene->mMeshes[0]->mAABB.mMax;
	for (unsigned int m = 1; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];
		if (min.x > mesh->mAABB.mMin.x) min.x = mesh->mAABB.mMin.x;
		if (min.y > mesh->mAABB.mMin.y) min.y = mesh->mAABB.mMin.y;
		if (min.z > mesh->mAABB.mMin.z) min.z = mesh->mAABB.mMin.z;
		if (max.x < mesh->mAABB.mMax.x) max.x = mesh->mAABB.mMax.x;
		if (max.y < mesh->mAABB.mMax.y) max.y = mesh->mAABB.mMax.y;
		if (max.z < mesh->mAABB.mMax.z) max.z = mesh->mAABB.mMax.z;
	}
	aabb.min.x = min.x + position.x;
	aabb.min.y = min.y + position.y;
	aabb.min.z = min.z + position.z;
	aabb.max.x = max.x + position.x;
	aabb.max.y = max.y + position.y;
	aabb.max.z = max.z + position.z;
	return aabb;
}






