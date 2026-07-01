#pragma once

enum class ResultOutcome
{
	MissionComplete,
	GameOver
};

void Result_SetOutcome(ResultOutcome outcome);
void Result_Initialize();
void Result_Finalize();
void Result_Update(double elapsed_time);
void Result_Draw();
