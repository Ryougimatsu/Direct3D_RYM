#pragma once

#include <DirectXMath.h>

#include <string>
#include <vector>

// Local transform used by weapon sockets and weapon attachment points.
// DirectXMath in this project uses row vectors: point * S * R * T.
struct WeaponSocketTransform
{
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 rotationDegrees = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	DirectX::XMMATRIX ToMatrix() const;
};

struct WeaponSocketProfile
{
	std::vector<std::string> boneCandidates = {
		"WeaponSocket",
		"weapon_socket",
		"mixamorig:RightHand",
		"RightHand",
		"Hand_R",
		"hand_r"
	};

	// Fine adjustment from the selected hand bone to the desired socket.
	WeaponSocketTransform adjustment;
};
