#include "sky.h"
using namespace DirectX;
#include "model.h"
#include "shader3d_unlit.h"

namespace {
	MODEL* g_pskyModel;
	XMFLOAT3 g_position;
}
void Sky_Initialize()
{
	g_pskyModel = ModelLoadS("resource/Model/sky.fbx",50.0f);
}

void Sky_Finalize()
{
	ModelRelease(g_pskyModel);
}

void Sky_SetPosition(const DirectX::XMFLOAT3& position)
{
	g_position = position;
}

void Sky_Draw()
{
	Shader3DUnilt_Begin();

	ModelUnlitDraw(g_pskyModel, XMMatrixTranslationFromVector(XMLoadFloat3(&g_position)));
}
