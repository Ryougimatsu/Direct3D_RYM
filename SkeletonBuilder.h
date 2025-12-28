// SkeletonBuilder.h
#pragma once

#include "skeleton.h"
#include <assimp/scene.h>
#include <DirectXMath.h>

Skeleton BuildSkeletonFromAssimp(const aiScene* scene);

// 如果你项目里还没有这个工具函数，可以一并声明
DirectX::XMMATRIX AiToXMMATRIX(const aiMatrix4x4& m);
