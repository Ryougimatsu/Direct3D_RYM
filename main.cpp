//#include <Windows.h>
#include <sstream>
#include "debug_ostream.h"
#include "game_window.h"


#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "direct3d.h"
#include "sprite.h"
#include "shader.h"
#include "shader_3d.h"
#include "sprite_anime.h"
#include "debug_text.h"
#include "texture.h"
#include <thread>
#include "system_timer.h"
#include <sstream>
#include "polygon.h"
#include "DirectXMath.h"
#include "key_logger.h"
#include "mouse.h"
#include "collision.h"
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
#include "game.h"
#include "Sampler.h"
#include "Audio.h"
#include "cube.h"
#include "fade.h"
#include "scene.h"
#include "Grid.h"
#include "camera.h"
#include "Meshfield.h"
#include "Light.h"

//Window procedure prototype claim
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE,_In_ LPSTR, _In_ int nCmdShow)
{
	(void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	HWND hWnd = GameWindow_Generate(hInstance);

	SystemTimer_Initialize();

	Direct3D_Initialize(hWnd);

	KeyLogger_Initialize();

	Mouse_Initialize(hWnd);

	InitAudio();

	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	Shader_3D_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	Texture_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	Polygon_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	Sprite_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	SpriteAnime_Initialize();

	Fade_Initialize();

	Scene_Initialize();
	
	//Grid_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	Cube_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	Sampler_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	Light_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	Light_SetPointLightCount(1);
	
	MeshField_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());



#if defined(_DEBUG) || defined(DEBUG)

	hal::DebugText dt(Direct3D_GetDevice(), Direct3D_GetDeviceContext(),
		L"resource/texture/consolab_ascii_512.png",
		Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight(),
		0.0f, 0.0f,
		0, 0,
		0.0f, 16.0f
	);

	Collision_DebugInitialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

#endif // _DEBUG || DEBUG

	//Game_Initialize();

	ShowWindow(hWnd, nCmdShow);

	UpdateWindow(hWnd);

	//Mouse_SetVisible(false);



	double exec_last_time = SystemTimer_GetTime();
	double fps_last_time = exec_last_time;
	double current_time = 0.0;
	ULONG frame_count = 0;
	double fps = 0.0;


	MSG msg;


	do{
		if (PeekMessage(&msg,nullptr,0,0,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

			current_time = SystemTimer_GetTime();
			double elapsed_time = current_time - fps_last_time;
			
			if (elapsed_time >= 1.0)
			{
				fps = frame_count / elapsed_time;
				fps_last_time = current_time;
				frame_count = 0;
			}

			elapsed_time = current_time - exec_last_time;
			//if (elapsed_time >= (1.0 / 60.0)) {  // 60FPSで更新する場合
			if (true){
				exec_last_time = current_time;


				//ゲームループ処理／ゲーム更新
				KeyLogger_Update();

				//Game_Update(elapsed_time);
				Scene_Update(elapsed_time);
				Fade_Update(elapsed_time);

				SpriteAnime_Update(elapsed_time);
				//ゲーム描画処理
				Direct3D_Clear();

				Sprite_Begin();

				//Game_Draw();
				Scene_Draw();
				Fade_Draw();


#if	defined(DEBUG) || defined(_DEBUG)
				std::stringstream ss;
				ss << "fps: " << fps << std::endl;
				dt.SetText(ss.str().c_str());
				dt.Draw();
				dt.Clear();
#endif


				Direct3D_Present();

				Scene_Refresh();

				frame_count++;
			}
		}
	} while (msg.message != WM_QUIT);

	//Game_Finalize();
#if defined(_DEBUG) || defined(DEBUG)

	Collision_DebugFinalize();

#endif

	Grid_Finalize();

	Cube_Finalize();

	Light_Finalize();
	
	MeshField_Finalize();

	Scene_Finalize();

	Fade_Finalize();

	SpriteAnime_Finalize();

	Texture_Finalize();

	Sprite_Finalize();

	Texture_AllRelease();

	Sprite_Finalize();

	Shader_Finalize();

	Shader_3D_Finalize();

	Polygon_Finalize();

	Direct3D_Finalize();

	Mouse_Finalize();

	return static_cast<int>(msg.wParam);

}


