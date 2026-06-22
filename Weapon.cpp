#include "Weapon.h"

#include <utility>

void Weapon::Draw(const DirectX::XMMATRIX& worldMatrix) const
{
	if (m_model)
	{
		ModelDraw(m_model, worldMatrix);
	}
}

void Weapon::DrawShadow(const DirectX::XMMATRIX& worldMatrix) const
{
	if (m_model)
	{
		ModelDrawShadow(m_model, worldMatrix);
	}
}

DirectX::XMMATRIX Weapon::GetGripLocalMatrix() const
{
	return m_gripPoint.ToMatrix();
}

DirectX::XMMATRIX Weapon::GetMuzzleLocalMatrix() const
{
	return m_muzzlePoint.ToMatrix();
}

DirectX::XMMATRIX Weapon::GetLaserLocalMatrix() const
{
	return m_laserPoint.ToMatrix();
}

DirectX::XMMATRIX Weapon::GetLeftHandLocalMatrix() const
{
	return m_leftHandPoint.ToMatrix();
}
