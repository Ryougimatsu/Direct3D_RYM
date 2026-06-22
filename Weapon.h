#pragma once

#include "WeaponSocket.h"
#include "model.h"

#include <DirectXMath.h>
#include <string>
#include <utility>

// Lightweight weapon instance. The MODEL resource remains owned by the
// existing shared asset system; this class owns only attachment metadata.
class Weapon
{
public:
	void SetName(std::string name) { m_name = std::move(name); }
	const std::string& GetName() const { return m_name; }

	void SetModel(MODEL* model) { m_model = model; }
	MODEL* GetModel() const { return m_model; }

	void Draw(const DirectX::XMMATRIX& worldMatrix) const;
	void DrawShadow(const DirectX::XMMATRIX& worldMatrix) const;

	WeaponSocketTransform& GripPoint() { return m_gripPoint; }
	WeaponSocketTransform& MuzzlePoint() { return m_muzzlePoint; }
	WeaponSocketTransform& LaserPoint() { return m_laserPoint; }
	WeaponSocketTransform& LeftHandPoint() { return m_leftHandPoint; }

	const WeaponSocketTransform& GripPoint() const { return m_gripPoint; }
	const WeaponSocketTransform& MuzzlePoint() const { return m_muzzlePoint; }
	const WeaponSocketTransform& LaserPoint() const { return m_laserPoint; }
	const WeaponSocketTransform& LeftHandPoint() const { return m_leftHandPoint; }

	DirectX::XMMATRIX GetGripLocalMatrix() const;
	DirectX::XMMATRIX GetMuzzleLocalMatrix() const;
	DirectX::XMMATRIX GetLaserLocalMatrix() const;
	DirectX::XMMATRIX GetLeftHandLocalMatrix() const;

private:
	std::string m_name;
	MODEL* m_model = nullptr;

	WeaponSocketTransform m_gripPoint;
	WeaponSocketTransform m_muzzlePoint;
	WeaponSocketTransform m_laserPoint;
	WeaponSocketTransform m_leftHandPoint;
};
