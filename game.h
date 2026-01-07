///ゲーム本体///

#ifndef GAME_H
#define GAME_H

#include <DirectXMath.h>
#include "collision.h"
#include "key_logger.h"
void Game_Initialize();
void Game_LoadContent();
void Game_Update(double elapsed_time);

void Game_Draw();

void Game_Finalize();

bool Game_CheckCollisionWithWalls(const AABB& objAabb);
bool Game_IsLineOfSightBlocked(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end);

#endif 