#pragma once

#include <cstdint>
#include <functional>
#include <vector>

// Tunable values for one player's experience curve and reward scaling.
struct ExperienceConfig
{
	std::uint64_t baseRequiredExperience = 100;
	double growthRate = 1.5;
	double experienceMultiplier = 1.0;

	// Enemy level - player level controls the reward multiplier.
	double higherLevelBonusPerLevel = 0.20;
	double lowerLevelPenaltyPerLevel = 0.15;
	double minimumLevelFactor = 0.10;
	double maximumLevelFactor = 3.00;

	// 0 means no configured cap.
	std::uint32_t maximumLevel = 0;
};

struct LevelUpEvent
{
	std::uint32_t previousLevel = 1;
	std::uint32_t newLevel = 1;
	std::uint64_t remainingExperience = 0;
	std::uint64_t experienceRequiredForNextLevel = 0;
};

struct ExperienceGainResult
{
	std::uint64_t baseExperience = 0;
	std::uint64_t awardedExperience = 0;
	double levelFactor = 1.0;
	std::uint32_t previousLevel = 1;
	std::uint32_t newLevel = 1;
	std::vector<LevelUpEvent> levelUps;
};

class ExperienceComponent
{
public:
	using LevelUpCallback = std::function<void(const LevelUpEvent&)>;

	explicit ExperienceComponent(ExperienceConfig config = {});

	// Calculates the level-adjusted reward, adds it, and returns all level-up events.
	ExperienceGainResult AddExperience(
		std::uint64_t enemyBaseExperience,
		std::uint32_t enemyLevel);

	// Useful for quests, pickups, debug commands, or other rewards with no enemy level.
	ExperienceGainResult AddRawExperience(std::uint64_t amount);

	std::uint64_t CalculateExperienceReward(
		std::uint64_t enemyBaseExperience,
		std::uint32_t enemyLevel) const;
	double CalculateLevelFactor(std::uint32_t enemyLevel) const;
	std::uint64_t CalculateRequiredExperience(std::uint32_t level) const;

	void SetLevelUpCallback(LevelUpCallback callback);
	void SetExperienceMultiplier(double multiplier);
	void Reset(std::uint32_t level = 1, std::uint64_t currentExperience = 0);

	std::uint32_t GetLevel() const { return m_level; }
	std::uint64_t GetCurrentExperience() const { return m_currentExperience; }
	std::uint64_t GetRequiredExperience() const;
	double GetExperienceMultiplier() const { return m_config.experienceMultiplier; }
	const ExperienceConfig& GetConfig() const { return m_config; }
	bool IsMaxLevel() const;

private:
	ExperienceGainResult ApplyExperience(
		std::uint64_t amount,
		std::uint64_t baseExperience,
		double levelFactor);
	void SanitizeConfig();

	ExperienceConfig m_config;
	std::uint32_t m_level = 1;
	std::uint64_t m_currentExperience = 0;
	LevelUpCallback m_levelUpCallback;
};
