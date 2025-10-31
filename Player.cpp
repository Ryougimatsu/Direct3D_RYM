#include "Player.h"
#include "model.h"
#include "key_logger.h"
#include "Light.h"
#include "camera.h"
#include "Player_Camera.h"

namespace {
	DirectX::XMFLOAT3 g_PlayerPosition = {};
	DirectX::XMFLOAT3 g_PlayerFront = { 0.0f,0.0f,1.0f };
	DirectX::XMFLOAT3 g_PlayerVelocity = {};
	MODEL* g_pPlayerModel = nullptr;
	bool g_IsJump = false;
}

void Player_Initialize(const DirectX :: XMFLOAT3& position,const DirectX::XMFLOAT3 front)
{
	g_PlayerPosition = position;
	g_PlayerVelocity = { 0.0f,0.0f,0.0f };
	DirectX::XMStoreFloat3(&g_PlayerFront, DirectX ::XMVector3Normalize(DirectX ::XMLoadFloat3(&front)));
	g_pPlayerModel = ModelLoad("resource/Model/test.fbx", 0.1f);
}

void Player_Finalize()
{
	ModelRelease(g_pPlayerModel);
}

void Player_Update(double elapsed_time)
{
	DirectX::XMVECTOR player_pos = DirectX::XMLoadFloat3(&g_PlayerPosition);
	DirectX::XMVECTOR player_velocity = DirectX::XMLoadFloat3(&g_PlayerVelocity);

	if (KeyLogger_IsTrigger(KK_SPACE) && !g_IsJump) {
		player_velocity += {0.0f, 10.0f, 0.0f};
		g_IsJump = true;
	}

	DirectX::XMVECTOR gdir{ 0.0f,1.0f,0.0f };
	player_velocity += gdir * -9.8f * 1.0f * static_cast<float>(elapsed_time);
	player_pos += player_velocity * static_cast<float>(elapsed_time);

	if (DirectX::XMVectorGetY(player_pos) < 1.0f)
	{
		player_pos -= player_velocity * static_cast<float>(elapsed_time);
		player_velocity *= { 1.0f,0.0f ,1.0f};
		g_IsJump = false;
	}
	
	DirectX::XMVECTOR direction{};
	DirectX::XMVECTOR front = DirectX::XMLoadFloat3(&Player_Camera_GetFront());

	if (KeyLogger_IsPressed(KK_W)) {
		direction += DirectX::XMLoadFloat3(&Player_Camera_GetFront());
	}

	if (KeyLogger_IsPressed(KK_S)) {
		direction -= DirectX::XMLoadFloat3(&Player_Camera_GetFront());
	}

	if (KeyLogger_IsPressed(KK_D)) {
		direction += DirectX::XMVector3Cross({ 0.0f,1.0f,0.0f }, front);
	}

	if (KeyLogger_IsPressed(KK_A)) {
		direction -= DirectX::XMVector3Cross({ 0.0f,1.0f,0.0f }, front);
	}


	direction = DirectX::XMVector2Normalize(direction);

	player_velocity += direction * (float)(80000.0 / 50.0 * elapsed_time);
	player_velocity += front * XMVECTOR{ -1.0f, 0.0f, -1.0f } *(float)(5.0 * elapsed_time);
	player_pos += player_velocity * (float)elapsed_time;



	DirectX::XMStoreFloat3(&g_PlayerPosition, player_pos);
	DirectX::XMStoreFloat3(&g_PlayerVelocity, player_velocity);
}

void Player_Draw()
{
	Light_SetSpecularWorld(Camera_GetPosition(), 10.0f, { 0.1f,0.1f,0.1f,0.1f });//镜面反射光
	DirectX ::XMMATRIX world = DirectX::XMMatrixTranslation(
		g_PlayerPosition.x,
		g_PlayerPosition.y,
		g_PlayerPosition.z
	);
	ModelDraw(g_pPlayerModel, world);
}

const DirectX::XMFLOAT3& Player_GetPosition()
{
	return g_PlayerPosition;
}

const DirectX::XMFLOAT3& Player_GetFront()
{
	return g_PlayerFront;
}
