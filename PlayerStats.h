#pragma once

// Centralized runtime stats for the player.  Skill effects should modify this
// object instead of scattering new numeric fields through Player/Bullet/Game.
struct PlayerStats
{
	float maxHp = 100.0f;
	float currentHp = 100.0f;
	float moveSpeed = 1.15f;
	float bulletDamage = 10.0f;
	int bulletPierce = 0;
	bool infiniteAmmo = false;
	float infiniteAmmoTimer = 0.0f;

	void ClampCurrentHp();
	void ApplyDamage(float damage);
	void Heal(float amount);
	void UpdateTimers(float deltaTime);
	bool IsInfiniteAmmoActive() const;
};
