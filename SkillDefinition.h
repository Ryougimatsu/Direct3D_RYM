#pragma once

#include <string>

enum class SkillType
{
	TemporaryInfiniteAmmo,
	IncreaseMaxHp,
	IncreaseBulletPierce,
	IncreaseBulletDamage,
	IncreaseMoveSpeed
};

struct SkillDefinition
{
	SkillType type = SkillType::IncreaseBulletDamage;
	std::wstring name;
	std::wstring description;
	int intValue = 0;
	float floatValue = 0.0f;
	int maxStack = 1;
};
