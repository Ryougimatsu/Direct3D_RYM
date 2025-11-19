#pragma once

#include <unordered_map>
#include <DirectXMath.h>
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"
#pragma comment (lib, "assimp-vc143-mt.lib")
#include "collision.h"

enum CategoryID
{
	Grass = 0,
	Brick = 1,
};

struct MODEL
{
	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer;
	ID3D11Buffer** IndexBuffer;

	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;
};


MODEL* ModelLoad(const char* FileName,float size);
MODEL* ModelLoadS(const char* FileName, float size);
void ModelRelease(MODEL* model);

void ModelDraw(MODEL* model,const DirectX ::XMMATRIX& mtxWorld );

AABB ModelGetAABB(MODEL* model, const DirectX::XMFLOAT3& position);

