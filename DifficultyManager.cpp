#include "DifficultyManager.h"

#include <algorithm>

namespace
{
	constexpr float SECONDS_PER_DIFFICULTY_LEVEL = 30.0f;
	constexpr int MIN_DIFFICULTY_LEVEL = 1;
	constexpr int MAX_DIFFICULTY_LEVEL = 30;

	constexpr float HP_MULTIPLIER_PER_LEVEL = 0.10f;
	constexpr float DAMAGE_MULTIPLIER_PER_LEVEL = 0.06f;
	constexpr float SPEED_MULTIPLIER_PER_LEVEL = 0.025f;

	constexpr float BASE_SPAWN_INTERVAL = 2.0f;
	constexpr float SPAWN_INTERVAL_REDUCTION_PER_LEVEL = 0.07f;
	constexpr float MIN_SPAWN_INTERVAL = 0.45f;

	constexpr int BASE_ENEMIES_PER_WAVE = 2;
	constexpr int ENEMY_COUNT_LEVEL_STEP = 3;
	constexpr int MAX_ENEMIES_PER_WAVE = 12;

	float GetLevelStep(int difficultyLevel)
	{
		return static_cast<float>(std::max(0, difficultyLevel - 1));
	}
}

void DifficultyManager::Reset()
{
	m_ElapsedTime = 0.0f;
	m_DifficultyLevel = MIN_DIFFICULTY_LEVEL;
}

void DifficultyManager::Update(float deltaTime)
{
	if (deltaTime <= 0.0f)
	{
		return;
	}

	m_ElapsedTime += deltaTime;

	const int calculatedLevel =
		MIN_DIFFICULTY_LEVEL +
		static_cast<int>(m_ElapsedTime / SECONDS_PER_DIFFICULTY_LEVEL);

	m_DifficultyLevel = std::clamp(
		calculatedLevel,
		MIN_DIFFICULTY_LEVEL,
		MAX_DIFFICULTY_LEVEL);
}

float DifficultyManager::GetEnemyHpMultiplier() const
{
	return 1.0f + GetLevelStep(m_DifficultyLevel) * HP_MULTIPLIER_PER_LEVEL;
}

float DifficultyManager::GetEnemyDamageMultiplier() const
{
	return 1.0f + GetLevelStep(m_DifficultyLevel) * DAMAGE_MULTIPLIER_PER_LEVEL;
}

float DifficultyManager::GetEnemySpeedMultiplier() const
{
	return 1.0f + GetLevelStep(m_DifficultyLevel) * SPEED_MULTIPLIER_PER_LEVEL;
}

float DifficultyManager::GetSpawnInterval() const
{
	const float interval =
		BASE_SPAWN_INTERVAL -
		GetLevelStep(m_DifficultyLevel) * SPAWN_INTERVAL_REDUCTION_PER_LEVEL;
	return std::max(MIN_SPAWN_INTERVAL, interval);
}

int DifficultyManager::GetEnemiesPerWave() const
{
	const int count =
		BASE_ENEMIES_PER_WAVE +
		(m_DifficultyLevel / ENEMY_COUNT_LEVEL_STEP);
	return std::min(MAX_ENEMIES_PER_WAVE, count);
}
