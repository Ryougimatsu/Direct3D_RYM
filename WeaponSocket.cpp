#include "WeaponSocket.h"

DirectX::XMMATRIX WeaponSocketTransform::ToMatrix() const
{
	using namespace DirectX;

	const XMMATRIX scaling =
		XMMatrixScaling(scale.x, scale.y, scale.z);
	const XMMATRIX rotation = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(rotationDegrees.x),
		XMConvertToRadians(rotationDegrees.y),
		XMConvertToRadians(rotationDegrees.z));
	const XMMATRIX translation =
		XMMatrixTranslation(position.x, position.y, position.z);

	return scaling * rotation * translation;
}
