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
	ClampAmmo();
	if (infiniteAmmoTimer <= 0.0f)
	{
		infiniteAmmo = false;
	}
}

void PlayerStats::ClampAmmo()
{
	magazineSize = std::max(1, magazineSize);
	maxReserveAmmo = std::max(0, maxReserveAmmo);
	currentAmmo = std::clamp(currentAmmo, 0, magazineSize);
	reserveAmmo = std::clamp(reserveAmmo, 0, maxReserveAmmo);
	itemDropRateBonus = std::clamp(itemDropRateBonus, 0.0f, 0.50f);
}

void PlayerStats::RefillAllAmmo()
{
	currentAmmo = magazineSize;
	reserveAmmo = maxReserveAmmo;
	ClampAmmo();
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
