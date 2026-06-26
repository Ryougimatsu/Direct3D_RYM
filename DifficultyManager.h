#pragma once

class DifficultyManager
{
public:
	void Reset();
	void Update(float deltaTime);

	float GetElapsedTime() const { return m_ElapsedTime; }
	int GetDifficultyLevel() const { return m_DifficultyLevel; }

	float GetEnemyHpMultiplier() const;
	float GetEnemyDamageMultiplier() const;
	float GetEnemySpeedMultiplier() const;
	float GetSpawnInterval() const;
	int GetEnemiesPerWave() const;

private:
	float m_ElapsedTime = 0.0f;
	int m_DifficultyLevel = 1;
};
