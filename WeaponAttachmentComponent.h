#pragma once

#include "Weapon.h"
#include "WeaponSocket.h"

#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <utility>

enum class WeaponAttachmentEditTarget
{
	Socket,
	GripPoint,
	MuzzlePoint
};

class WeaponAttachmentComponent
{
public:
	void AttachWeapon(Weapon* weapon);
	void SetSocketProfile(WeaponSocketProfile profile);
	WeaponSocketProfile& GetSocketProfile() { return m_socketProfile; }
	const WeaponSocketProfile& GetSocketProfile() const { return m_socketProfile; }

	bool ResolveSocketBone(
		const std::unordered_map<std::string, int>& boneNameToIndex);

	// handBoneModelMatrix is supplied by Animator; characterWorldMatrix moves
	// that model-space bone into world space.
	void Update(
		const DirectX::XMMATRIX& handBoneModelMatrix,
		const DirectX::XMMATRIX& characterWorldMatrix);

	bool IsValid() const { return m_weapon != nullptr && m_boneIndex >= 0; }
	int GetBoneIndex() const { return m_boneIndex; }
	const std::string& GetResolvedBoneName() const { return m_resolvedBoneName; }

	const DirectX::XMMATRIX& GetHandWorldMatrix() const { return m_handWorld; }
	const DirectX::XMMATRIX& GetSocketWorldMatrix() const { return m_socketWorld; }
	const DirectX::XMMATRIX& GetWeaponWorldMatrix() const { return m_weaponWorld; }
	const DirectX::XMMATRIX& GetGripWorldMatrix() const { return m_gripWorld; }
	const DirectX::XMMATRIX& GetMuzzleWorldMatrix() const { return m_muzzleWorld; }
	const DirectX::XMMATRIX& GetLaserWorldMatrix() const { return m_laserWorld; }
	const DirectX::XMMATRIX& GetLeftHandWorldMatrix() const { return m_leftHandWorld; }

	DirectX::XMVECTOR GetMuzzleWorldPosition() const;
	DirectX::XMVECTOR GetMuzzleForward() const;
	DirectX::XMVECTOR GetLaserWorldPosition() const;

	void SetConfigPath(std::string path) { m_configPath = std::move(path); }
	const std::string& GetConfigPath() const { return m_configPath; }
	bool SaveConfig() const;
	bool LoadConfig();

	// F6: debug display, F7: save, F8: reload, F9: edit target.
	// Arrows/PageUp/PageDown move. Shift rotates. Home/End scale.
	bool UpdateRuntimeDebugControls(double elapsedTime);
	bool IsDebugVisible() const { return m_debugVisible; }
	WeaponAttachmentEditTarget GetEditTarget() const { return m_editTarget; }

private:
	WeaponSocketTransform* GetEditableTransform();
	const WeaponSocketTransform* GetEditableTransform() const;
	void PrintDebugState() const;

	Weapon* m_weapon = nullptr;
	WeaponSocketProfile m_socketProfile;
	int m_boneIndex = -1;
	std::string m_resolvedBoneName;
	std::string m_configPath = "resource/config/M4A4_weapon_socket.txt";

	DirectX::XMMATRIX m_handWorld = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_socketWorld = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_weaponWorld = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_gripWorld = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_muzzleWorld = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_laserWorld = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_leftHandWorld = DirectX::XMMatrixIdentity();

	bool m_debugVisible = false;
	WeaponAttachmentEditTarget m_editTarget =
		WeaponAttachmentEditTarget::Socket;
};
