#include "Player.h"
#include "model.h"
#include "key_logger.h"
#include "Light.h"
#include "camera.h"
#include "Player_Camera.h"
#include "cube.h"
using namespace DirectX;

namespace {
	DirectX::XMFLOAT3 g_PlayerPosition = {};
	DirectX::XMFLOAT3 g_PlayerFront = { 0.0f,0.0f,1.0f };
	DirectX::XMFLOAT3 g_PlayerVelocity = {};
	MODEL* g_pPlayerModel = nullptr;
	bool g_IsJump = false;
}

void Player_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3 front)
{
	g_PlayerPosition = position;
	g_PlayerVelocity = { 0.0f,0.0f,0.0f };
	DirectX::XMStoreFloat3(&g_PlayerFront, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&front)));
	g_pPlayerModel = ModelLoad("resource/Model/test.fbx", 0.05f);
}

void Player_Finalize()
{
	ModelRelease(g_pPlayerModel);
}

void Player_Update(double elapsed_time)
{
	XMVECTOR position = XMLoadFloat3(&g_PlayerPosition);
	XMVECTOR velocity = XMLoadFloat3(&g_PlayerVelocity);

	//移動
	if (KeyLogger_IsTrigger(KK_SPACE) && !g_IsJump)
	{
		velocity += {0.0f, 8.0f, 0.0f };
		g_IsJump = true;
	}

	//重力
	XMVECTOR gravityDir = { 0.0f,-1.0f,0.0f };
	velocity += gravityDir * 9.8f * 1.5f * static_cast<float>(elapsed_time);
	position += velocity * static_cast<float>(elapsed_time);


	XMStoreFloat3(&g_PlayerPosition, position);
	AABB player = Player_GetAABB();
	AABB cube = Cube_CreateAABB({ 3.0f, 3.0f, 2.0f });

	Hit hit = Collision_IsHitAABB(cube, player);
	if (hit.isHit) {
		if (hit.normal.y > 0.0f) { // 上にたぶんのっかった
			// position -= gvelocity; // 落ちたことをなかったことにする
			XMVectorSetY(position, cube.max.y + 0.01f);
			// gvelocity = {};
			velocity *= { 1.0f, 0.0f, 1.0f };
			g_IsJump = false;
		}
	}
	else if (XMVectorGetY(position) < 0.5f) {
		position -= gravityDir; // 落ちたことをなかったことにする
		// gvelocity = {};
		velocity *= { 1.0f, 0.0f, 1.0f };
		g_IsJump = false;
	}

	XMVECTOR direction{};
	XMVECTOR front = XMLoadFloat3(&Player_Camera_GetFront()) * XMVECTOR { 1.0f, 0.0f, 1.0f };

	if (KeyLogger_IsPressed(KK_W))
	{
		direction += front;
	}
	if (KeyLogger_IsPressed(KK_S))
	{
		direction -= front;
	}


	if (KeyLogger_IsPressed(KK_A))
	{
		direction -= XMVector3Cross({ 0.0f,1.0f,0.0f }, front);

	}
	if (KeyLogger_IsPressed(KK_D))
	{
		direction += XMVector3Cross({ 0.0f,1.0f,0.0f }, front);
	}

	if (XMVectorGetX(XMVector3LengthSq(direction)) > 0.0f) { // 如果有按键输入

		direction = XMVector3Normalize(direction); // 'direction' 是我们的 "目标朝向"
		XMVECTOR currentFront = XMLoadFloat3(&g_PlayerFront);


		// --- 这是新的 180 度转向修复 ---

		// 'interpTarget' 是我们这一帧真正要插值的目标
		// 默认情况下，它就是我们想去的方向
		XMVECTOR interpTarget = direction;

		// 1. 计算当前朝向和目标朝向的点积
		float dot = XMVectorGetX(DirectX::XMVector3Dot(currentFront, direction));

		// 2. 检查它们是否几乎完全相反 (点积接近 -1)
		if (dot < -0.9999f)
		{
			// 危险！我们不能直接插值到 'direction'
			// 我们需要找一个 "绕路" 的向量。玩家的 "右" 向量是最好的选择。
			XMVECTOR right = DirectX::XMVector3Cross({ 0.0f, 1.0f, 0.0f }, currentFront);

			// （安全检查）如果玩家恰好面朝上（虽然在你的游戏里不太可能）
			// 'right' 可能是 0。这种情况下我们随便用一个X轴。
			if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(right)) < 0.001f)
			{
				right = { 1.0f, 0.0f, 0.0f };
			}

			// 3. 将我们这一帧的插值目标 'interpTarget' 改为 "right"
			// 这会使玩家开始向侧面转向
			interpTarget = DirectX::XMVector3Normalize(right);
		}
		// --- 修复结束 ---


		// (下面的代码和以前一样, 只是把 'direction' 换成了 'interpTarget')

		const float ROTATION_SMOOTH_FACTOR = 10.0f;
		float interpAmount = ROTATION_SMOOTH_FACTOR * static_cast<float>(elapsed_time);
		interpAmount = (interpAmount > 1.0f) ? 1.0f : interpAmount;

		// 5. 使用 LERP (线性插值)，但目标是 'interpTarget'
		XMVECTOR newFront = DirectX::XMVectorLerp(currentFront, interpTarget, interpAmount);

		// 6. 重新标准化 (将 LERP 变为 NLerp)
		newFront = DirectX::XMVector3Normalize(newFront);

		// 7. 使用这个新的、平滑过渡的朝向来增加速度
		velocity += newFront * static_cast<float>(2000.0 / 50.0 * elapsed_time);

		// 8. 存储更新后的朝向
		DirectX::XMStoreFloat3(&g_PlayerFront, newFront);
	}

	velocity += -velocity * static_cast<float>(4.0f * elapsed_time);
	position += velocity * static_cast<float>(elapsed_time);


	DirectX::XMStoreFloat3(&g_PlayerPosition, position);
	DirectX::XMStoreFloat3(&g_PlayerVelocity, velocity);


}

void Player_Draw()
{
	Light_SetSpecularWorld(Camera_GetPosition(), 10.0f, { 0.1f,0.1f,0.1f,0.1f });//镜面反射光

	float angle = -atan2f(g_PlayerFront.z, g_PlayerFront.x) + XMConvertToRadians(270);

	// 1. 创建旋转矩阵
	XMMATRIX r = XMMatrixRotationY(angle);

	// 2. 创建平移矩阵
	DirectX::XMMATRIX t = DirectX::XMMatrixTranslation(
		g_PlayerPosition.x,
		g_PlayerPosition.y,
		g_PlayerPosition.z
	);

	// 3. 组合矩阵 (先旋转，再平移)
	DirectX::XMMATRIX world = r * t; // <--- 这是关键的修改

	// 4. 使用组合后的 world 矩阵绘制模型
	ModelDraw(g_pPlayerModel, world);
}

AABB Player_GetAABB()
{
	return{
		{ g_PlayerPosition.x - 1.0f,
		  g_PlayerPosition.y,
		  g_PlayerPosition.z - 1.0f,
		},
		{ g_PlayerPosition.x + 1.0f,
		  g_PlayerPosition.y + 2.0f,
		  g_PlayerPosition.z + 1.0f
		}
	};
}


const DirectX::XMFLOAT3& Player_GetPosition()
{
	return g_PlayerPosition;
}

const DirectX::XMFLOAT3& Player_GetFront()
{
	return g_PlayerFront;
}