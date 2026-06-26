#include "PlayerStats.h"

#include <algorithm>

void PlayerStats::ClampCurrentHp()
{
	maxHp = std::max(1.0f, maxHp);
	currentHp = std::clamp(currentHp, 0.0f, maxHp);
	moveSpeed = std::max(0.0f, moveSpeed);
	bulletDamage = std::max(0.0f, bulletDamage);
	bulletPierce = std::max(0, bulletPierce);
	infiniteAmmoTimer = std::max(0.0f, infiniteAmmoTimer);
	if (infiniteAmmoTimer <= 0.0f)
	{
		infiniteAmmo = false;
	}
}

void PlayerStats::ApplyDamage(float damage)
{
	if (damage <= 0.0f)
	{
		return;
	}

	currentHp -= damage;
	ClampCurrentHp();
}

void PlayerStats::Heal(float amount)
{
	if (amount <= 0.0f || currentHp <= 0.0f)
	{
		return;
	}

	currentHp += amount;
	ClampCurrentHp();
}

void PlayerStats::UpdateTimers(float deltaTime)
{
	if (deltaTime <= 0.0f)
	{
		ClampCurrentHp();
		return;
	}

	if (infiniteAmmoTimer > 0.0f)
	{
		infiniteAmmoTimer -= deltaTime;
		if (infiniteAmmoTimer <= 0.0f)
		{
			infiniteAmmoTimer = 0.0f;
			infiniteAmmo = false;
		}
	}

	ClampCurrentHp();
}

bool PlayerStats::IsInfiniteAmmoActive() const
{
	return infiniteAmmo || infiniteAmmoTimer > 0.0f;
}
