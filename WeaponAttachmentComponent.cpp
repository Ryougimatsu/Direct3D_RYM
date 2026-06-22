#include "WeaponAttachmentComponent.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace
{
	std::string NormalizeBoneName(const std::string& name)
	{
		std::string normalized;
		normalized.reserve(name.size());
		for (const unsigned char character : name)
		{
			if (std::isalnum(character))
			{
				normalized.push_back(
					static_cast<char>(std::tolower(character)));
			}
		}
		return normalized;
	}

	void WriteTransform(
		std::ostream& stream,
		const char* prefix,
		const WeaponSocketTransform& transform)
	{
		stream << prefix << ".position="
			<< transform.position.x << ','
			<< transform.position.y << ','
			<< transform.position.z << '\n';
		stream << prefix << ".rotation="
			<< transform.rotationDegrees.x << ','
			<< transform.rotationDegrees.y << ','
			<< transform.rotationDegrees.z << '\n';
		stream << prefix << ".scale="
			<< transform.scale.x << ','
			<< transform.scale.y << ','
			<< transform.scale.z << '\n';
	}

	bool ParseFloat3(
		const std::string& value,
		DirectX::XMFLOAT3& result)
	{
		std::istringstream stream(value);
		char commaA = 0;
		char commaB = 0;
		return static_cast<bool>(
			stream >> result.x >> commaA >>
			result.y >> commaB >> result.z) &&
			commaA == ',' && commaB == ',';
	}

	void ApplyConfigValue(
		const std::string& key,
		const std::string& value,
		WeaponSocketTransform& socket,
		WeaponSocketTransform& grip,
		WeaponSocketTransform& muzzle,
		WeaponSocketTransform& laser,
		WeaponSocketTransform& leftHand)
	{
		auto apply = [&](const char* prefix, WeaponSocketTransform& transform)
		{
			const std::string base(prefix);
			if (key == base + ".position")
			{
				ParseFloat3(value, transform.position);
			}
			else if (key == base + ".rotation")
			{
				ParseFloat3(value, transform.rotationDegrees);
			}
			else if (key == base + ".scale")
			{
				ParseFloat3(value, transform.scale);
			}
		};

		apply("socket", socket);
		apply("grip", grip);
		apply("muzzle", muzzle);
		apply("laser", laser);
		apply("left_hand", leftHand);
	}
}

void WeaponAttachmentComponent::AttachWeapon(Weapon* weapon)
{
	m_weapon = weapon;
}

void WeaponAttachmentComponent::SetSocketProfile(WeaponSocketProfile profile)
{
	m_socketProfile = std::move(profile);
}

bool WeaponAttachmentComponent::ResolveSocketBone(
	const std::unordered_map<std::string, int>& boneNameToIndex)
{
	m_boneIndex = -1;
	m_resolvedBoneName.clear();

	for (const std::string& candidate : m_socketProfile.boneCandidates)
	{
		const auto exact = boneNameToIndex.find(candidate);
		if (exact != boneNameToIndex.end())
		{
			m_boneIndex = exact->second;
			m_resolvedBoneName = exact->first;
			return true;
		}
	}

	for (const std::string& candidate : m_socketProfile.boneCandidates)
	{
		const std::string normalizedCandidate =
			NormalizeBoneName(candidate);
		for (const auto& [boneName, boneIndex] : boneNameToIndex)
		{
			if (NormalizeBoneName(boneName) == normalizedCandidate)
			{
				m_boneIndex = boneIndex;
				m_resolvedBoneName = boneName;
				return true;
			}
		}
	}

	return false;
}

void WeaponAttachmentComponent::Update(
	const DirectX::XMMATRIX& handBoneModelMatrix,
	const DirectX::XMMATRIX& characterWorldMatrix)
{
	using namespace DirectX;

	if (!m_weapon)
	{
		return;
	}

	m_handWorld = handBoneModelMatrix * characterWorldMatrix;
	m_socketWorld =
		m_socketProfile.adjustment.ToMatrix() * m_handWorld;

	// Row-vector convention:
	// gripLocal * weaponWorld == socketWorld
	// therefore weaponWorld == inverse(gripLocal) * socketWorld.
	XMVECTOR determinant;
	XMMATRIX inverseGrip = XMMatrixInverse(
		&determinant,
		m_weapon->GetGripLocalMatrix());
	if (std::abs(XMVectorGetX(determinant)) < 0.000001f)
	{
		inverseGrip = XMMatrixIdentity();
	}
	m_weaponWorld = inverseGrip * m_socketWorld;

	m_gripWorld = m_weapon->GetGripLocalMatrix() * m_weaponWorld;
	m_muzzleWorld = m_weapon->GetMuzzleLocalMatrix() * m_weaponWorld;
	m_laserWorld = m_weapon->GetLaserLocalMatrix() * m_weaponWorld;
	m_leftHandWorld =
		m_weapon->GetLeftHandLocalMatrix() * m_weaponWorld;
}

DirectX::XMVECTOR WeaponAttachmentComponent::GetMuzzleWorldPosition() const
{
	return m_muzzleWorld.r[3];
}

DirectX::XMVECTOR WeaponAttachmentComponent::GetMuzzleForward() const
{
	return DirectX::XMVector3Normalize(m_muzzleWorld.r[0]);
}

DirectX::XMVECTOR WeaponAttachmentComponent::GetLaserWorldPosition() const
{
	return m_laserWorld.r[3];
}

bool WeaponAttachmentComponent::SaveConfig() const
{
	if (!m_weapon || m_configPath.empty())
	{
		return false;
	}

	const std::filesystem::path path(m_configPath);
	if (path.has_parent_path())
	{
		std::error_code error;
		std::filesystem::create_directories(path.parent_path(), error);
	}

	std::ofstream stream(path);
	if (!stream)
	{
		return false;
	}

	stream << "weapon=" << m_weapon->GetName() << '\n';
	stream << "bone=" << m_resolvedBoneName << '\n';
	WriteTransform(stream, "socket", m_socketProfile.adjustment);
	WriteTransform(stream, "grip", m_weapon->GripPoint());
	WriteTransform(stream, "muzzle", m_weapon->MuzzlePoint());
	WriteTransform(stream, "laser", m_weapon->LaserPoint());
	WriteTransform(stream, "left_hand", m_weapon->LeftHandPoint());
	return true;
}

bool WeaponAttachmentComponent::LoadConfig()
{
	if (!m_weapon || m_configPath.empty())
	{
		return false;
	}

	std::ifstream stream(m_configPath);
	if (!stream)
	{
		return false;
	}

	std::string line;
	while (std::getline(stream, line))
	{
		const std::size_t separator = line.find('=');
		if (separator == std::string::npos)
		{
			continue;
		}

		const std::string key = line.substr(0, separator);
		const std::string value = line.substr(separator + 1);
		if (key == "bone" && !value.empty())
		{
			auto& candidates = m_socketProfile.boneCandidates;
			candidates.erase(
				std::remove(candidates.begin(), candidates.end(), value),
				candidates.end());
			candidates.insert(candidates.begin(), value);
			continue;
		}
		ApplyConfigValue(
			key,
			value,
			m_socketProfile.adjustment,
			m_weapon->GripPoint(),
			m_weapon->MuzzlePoint(),
			m_weapon->LaserPoint(),
			m_weapon->LeftHandPoint());
	}

	return true;
}

bool WeaponAttachmentComponent::UpdateRuntimeDebugControls(double elapsedTime)
{
	if ((GetAsyncKeyState(VK_F6) & 1) != 0)
	{
		m_debugVisible = !m_debugVisible;
		PrintDebugState();
	}
	if ((GetAsyncKeyState(VK_F7) & 1) != 0)
	{
		SaveConfig();
	}
	if ((GetAsyncKeyState(VK_F8) & 1) != 0)
	{
		LoadConfig();
		PrintDebugState();
		return true;
	}
	if ((GetAsyncKeyState(VK_F9) & 1) != 0)
	{
		const int next =
			(static_cast<int>(m_editTarget) + 1) % 3;
		m_editTarget =
			static_cast<WeaponAttachmentEditTarget>(next);
		PrintDebugState();
	}

	if (!m_debugVisible)
	{
		return false;
	}

	WeaponSocketTransform* transform = GetEditableTransform();
	if (!transform)
	{
		return false;
	}

	const float dt = static_cast<float>(elapsedTime);
	const bool rotate =
		(GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	bool changed = false;

	auto axisInput = [](int positiveKey, int negativeKey)
	{
		float value = 0.0f;
		if ((GetAsyncKeyState(positiveKey) & 0x8000) != 0) value += 1.0f;
		if ((GetAsyncKeyState(negativeKey) & 0x8000) != 0) value -= 1.0f;
		return value;
	};

	const float x = axisInput(VK_RIGHT, VK_LEFT);
	const float y = axisInput(VK_PRIOR, VK_NEXT);
	const float z = axisInput(VK_UP, VK_DOWN);

	if (x != 0.0f || y != 0.0f || z != 0.0f)
	{
		if (rotate)
		{
			const float rotationSpeed = 45.0f;
			transform->rotationDegrees.x += y * rotationSpeed * dt;
			transform->rotationDegrees.y += x * rotationSpeed * dt;
			transform->rotationDegrees.z += z * rotationSpeed * dt;
		}
		else
		{
			const float positionSpeed =
				m_editTarget == WeaponAttachmentEditTarget::Socket
				? 5.0f
				: 20.0f;
			transform->position.x += x * positionSpeed * dt;
			transform->position.y += y * positionSpeed * dt;
			transform->position.z += z * positionSpeed * dt;
		}
		changed = true;
	}

	float scaleInput = 0.0f;
	if ((GetAsyncKeyState(VK_HOME) & 0x8000) != 0) scaleInput += 1.0f;
	if ((GetAsyncKeyState(VK_END) & 0x8000) != 0) scaleInput -= 1.0f;
	if (scaleInput != 0.0f)
	{
		const float factor = std::max(0.01f, 1.0f + scaleInput * dt);
		transform->scale.x = std::max(0.001f, transform->scale.x * factor);
		transform->scale.y = std::max(0.001f, transform->scale.y * factor);
		transform->scale.z = std::max(0.001f, transform->scale.z * factor);
		changed = true;
	}

	return changed;
}

WeaponSocketTransform* WeaponAttachmentComponent::GetEditableTransform()
{
	if (!m_weapon)
	{
		return nullptr;
	}

	switch (m_editTarget)
	{
	case WeaponAttachmentEditTarget::Socket:
		return &m_socketProfile.adjustment;
	case WeaponAttachmentEditTarget::GripPoint:
		return &m_weapon->GripPoint();
	case WeaponAttachmentEditTarget::MuzzlePoint:
		return &m_weapon->MuzzlePoint();
	default:
		return nullptr;
	}
}

const WeaponSocketTransform*
WeaponAttachmentComponent::GetEditableTransform() const
{
	return const_cast<WeaponAttachmentComponent*>(this)
		->GetEditableTransform();
}

void WeaponAttachmentComponent::PrintDebugState() const
{
	const WeaponSocketTransform* transform = GetEditableTransform();
	if (!transform)
	{
		return;
	}

	const char* targetName = "Socket";
	if (m_editTarget == WeaponAttachmentEditTarget::GripPoint)
	{
		targetName = "GripPoint";
	}
	else if (m_editTarget == WeaponAttachmentEditTarget::MuzzlePoint)
	{
		targetName = "MuzzlePoint";
	}

	char text[512];
	sprintf_s(
		text,
		"[WeaponAttachment] Debug=%s Target=%s Bone=%s "
		"Pos=(%.3f, %.3f, %.3f) Rot=(%.2f, %.2f, %.2f) "
		"Scale=(%.3f, %.3f, %.3f)\n",
		m_debugVisible ? "On" : "Off",
		targetName,
		m_resolvedBoneName.c_str(),
		transform->position.x,
		transform->position.y,
		transform->position.z,
		transform->rotationDegrees.x,
		transform->rotationDegrees.y,
		transform->rotationDegrees.z,
		transform->scale.x,
		transform->scale.y,
		transform->scale.z);
	OutputDebugStringA(text);
}
