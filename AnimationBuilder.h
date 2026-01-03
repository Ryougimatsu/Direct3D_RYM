// AnimationBuilder.h
#pragma once
#include "Animation.h"
#include "Skeleton.h"
#include <assimp/scene.h>

Animation BuildAnimationFromAssimp(const aiScene* scene, int animIndex = 0);
