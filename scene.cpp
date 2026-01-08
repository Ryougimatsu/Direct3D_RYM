#include "scene.h"
#include "loading.h"
#include "game.h"
#include "title.h"
#include "gameover.h"
#include "result.h"
namespace 
{
	scene g_CurrentScene = SCENE_TITLE;

	scene g_NextScene = g_CurrentScene;
}
void Scene_Initialize()
{
	switch (g_CurrentScene)
	{
	case SCENE_TITLE:
		Title_Initialize();
		break;
	case SCENE_LOADING: 
		Loading_Initialize(); 
		break;
	case SCENE_GAME:
		Game_Initialize();
		break;
	case SCENE_RESULT:
		Result_Initialize();
		break;
	case SCENE_GAMEOVER: 
		GameOver_Initialize(); 
		break;
	default:
		break;
	}
}

void Scene_Finalize()
{
	switch (g_CurrentScene)
	{
	case SCENE_TITLE:
		Title_Finalize();
		break;
	case SCENE_LOADING: 
		Loading_Finalize(); 
		break;
	case SCENE_GAME:
		Game_Finalize();
		break;
	case SCENE_RESULT:
		Result_Finalize();
		break;
	case SCENE_GAMEOVER: 
		GameOver_Finalize();
		break;
	default:
		break;
	}
}

void Scene_Update(double elapsed_time)
{
	switch (g_CurrentScene)
	{
	case SCENE_TITLE:
		Title_Update(elapsed_time);
		break;
	case SCENE_LOADING: 
		Loading_Update(elapsed_time); 
		break;
	case SCENE_GAME:
		Game_Update(elapsed_time);
		break;
	case SCENE_RESULT:
		Result_Update(elapsed_time);
		break;
	case SCENE_GAMEOVER:
		GameOver_Update(elapsed_time);
		break;
	default:
		break;
	}
}

void Scene_Draw()
{
	switch (g_CurrentScene)
	{
	case SCENE_TITLE:
		Title_Draw();
		break;
	case SCENE_LOADING: 
		Loading_Draw(); 
		break;
	case SCENE_GAME:

		Game_Draw();
		break;
	case SCENE_RESULT:
		Result_Draw();
		break;
	case SCENE_GAMEOVER: 
		GameOver_Draw(); 
		break;
	default:
		break;
	}
}

void Scene_Refresh()
{
	if (g_CurrentScene != g_NextScene)
	{
		Scene_Finalize();
		g_CurrentScene = g_NextScene;
		Scene_Initialize();
	}
}

void Scene_Change(scene scene)
{
	g_NextScene = scene; // 現在のシーンを更新
}
