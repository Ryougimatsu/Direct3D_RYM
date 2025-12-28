#include "GameObject.h"

GameObject::GameObject()
{
}

GameObject::GameObject(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rotate, DirectX::XMFLOAT3 scale)
	:transform(pos, rotate, scale)
{
}

GameObject::GameObject(DirectX::XMFLOAT3 pos) :transform(pos, { 0,0,0 }, { 1,1,1 })
{
}

GameObject::GameObject(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rotate) :transform(pos, rotate, { 1,1,1 })
{
}

GameObject::~GameObject()
{
}

Transform::Transform()
{
}

Transform::Transform(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rotate, DirectX::XMFLOAT3 scale)
	:Position(pos), Rotation(rotate), Scale(scale)
{
}

Transform::~Transform()
{
}