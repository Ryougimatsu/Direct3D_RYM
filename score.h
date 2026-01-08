#pragma once
#ifndef SCORE_H
#define SCORE_H
void Score_Initialize(float x,float y,int digit);
void Score_Finalize();
void Score_Draw();
void Score_Update();
unsigned int Score_GetScore();
void Score_AddScore(int score);
void Score_Reset();
void Score_SetPosition(float x, float y);
void Score_SetTime(double time);
double Score_GetTime();
#endif