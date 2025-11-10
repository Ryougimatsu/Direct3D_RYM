#include "Player.h"
#include "model.h"
#include "key_logger.h"
#include "Light.h"
#include "camera.h"
#include "Player_Camera.h"
#include "map.h"
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
	float dt = static_cast<float>(elapsed_time);
	XMVECTOR position = XMLoadFloat3(&g_PlayerPosition);
	XMVECTOR velocity = XMLoadFloat3(&g_PlayerVelocity);

	// ==============================================================================
	// 1. 物理模拟阶段 (累加所有力，更新速度)
	// ==============================================================================

	// [重力]
	velocity += XMVECTOR{ 0.0f, -9.8f * 1.5f, 0.0f } *dt;

	// [跳跃输入]
	if (KeyLogger_IsTrigger(KK_SPACE) && !g_IsJump) {
		velocity = XMVectorSetY(velocity, 8.0f); // 直接设置Y速度更稳定
		g_IsJump = true;
	}

	// [移动输入 WASD]
	XMVECTOR moveDir = XMVectorZero();
	XMVECTOR front = XMVector3Normalize(XMLoadFloat3(&Player_Camera_GetFront()) * XMVECTOR { 1.0f, 0.0f, 1.0f });
	XMVECTOR right = XMVector3Normalize(XMVector3Cross({ 0.0f, 1.0f, 0.0f }, front));

	if (KeyLogger_IsPressed(KK_W)) moveDir += front;
	if (KeyLogger_IsPressed(KK_S)) moveDir -= front;
	if (KeyLogger_IsPressed(KK_D)) moveDir += right;
	if (KeyLogger_IsPressed(KK_A)) moveDir -= right;

	if (XMVectorGetX(XMVector3LengthSq(moveDir)) > 0.001f) {
		moveDir = XMVector3Normalize(moveDir);

		// --- 转向平滑 (保持不变) ---
		XMVECTOR currentFront = XMLoadFloat3(&g_PlayerFront);
		float dot = XMVectorGetX(XMVector3Dot(currentFront, moveDir));
		XMVECTOR targetFront = moveDir;
		if (dot < -0.999f) targetFront = XMVector3Normalize(XMVector3Cross({ 0.0f, 1.0f, 0.0f }, currentFront)); // 180度转向修复

		float interpAmount = (std::min)(10.0f * dt, 1.0f);
		XMVECTOR newFront = XMVector3Normalize(XMVectorLerp(currentFront, targetFront, interpAmount));
		XMStoreFloat3(&g_PlayerFront, newFront);
		// ------------------------

		// [应用移动加速度]
		velocity += newFront * (2000.0f / 50.0f) * dt;
	}

	// [阻力] (仅在水平方向应用，避免影响重力下落)
	XMVECTOR velocityY = velocity * XMVECTOR{ 0.f, 1.f, 0.f };
	XMVECTOR velocityXZ = velocity * XMVECTOR{ 1.f, 0.f, 1.f };
	velocityXZ *= (1.0f - (std::min)(10.0f * dt, 1.0f)); // 使用乘法阻尼更稳定
	velocity = velocityXZ + velocityY;

	// ==============================================================================
	// 2. 位置更新阶段 (尝试移动)
	// ==============================================================================
	position += velocity * dt;

	// ==============================================================================
	// 3. 碰撞解决阶段 (修正位置)
	// ==============================================================================

	// 先把最新的预测位置存回去，这样 Player_GetAABB() 才能拿到最新数据
	XMStoreFloat3(&g_PlayerPosition, position);

	int mapObjCount = Map_GetObjectsCount();
	for (int i = 0; i < mapObjCount; ++i) {
		// 每次循环都重新获取最新的玩家 AABB，因为前一次循环可能已经推挤了玩家
		AABB playerAABB = Player_GetAABB();
		AABB cubeAABB = Cube_CreateAABB(Map_GetObject(i)->Position);

		Hit hit = Collision_IsHitAABB(cubeAABB, playerAABB);

		if (hit.isHit) {
			// [重要修正] 必须把返回值赋回给 position
			if (hit.normal.y > 0.0f) { // 脚下
				if (XMVectorGetY(velocity) <= 0.0f) { // 只在下落时修正
					position = XMVectorSetY(position, cubeAABB.max.y + 0.001f);
					velocity = XMVectorSetY(velocity, 0.0f);
					g_IsJump = false;
				}
			}
			else if (hit.normal.y < 0.0f) { // 头顶
				if (XMVectorGetY(velocity) > 0.0f) { // 只在上升时修正
					position = XMVectorSetY(position, cubeAABB.min.y - 2.001f);
					velocity = XMVectorSetY(velocity, 0.0f);
				}
			}
			else if (hit.normal.x > 0.0f) { // X+ 面 (左侧撞墙)
				position = XMVectorSetX(position, cubeAABB.max.x + 1.001f);
				velocity = XMVectorSetX(velocity, 0.0f);
			}
			else if (hit.normal.x < 0.0f) { // X- 面 (右侧撞墙)
				position = XMVectorSetX(position, cubeAABB.min.x - 1.001f);
				velocity = XMVectorSetX(velocity, 0.0f);
			}
			else if (hit.normal.z > 0.0f) { // Z+ 面 (后侧撞墙)
				position = XMVectorSetZ(position, cubeAABB.max.z + 1.001f);
				velocity = XMVectorSetZ(velocity, 0.0f);
			}
			else if (hit.normal.z < 0.0f) { // Z- 面 (前侧撞墙)
				position = XMVectorSetZ(position, cubeAABB.min.z - 1.001f);
				velocity = XMVectorSetZ(velocity, 0.0f);
			}

			// 更新位置以便下一次检测使用最新数据
			XMStoreFloat3(&g_PlayerPosition, position);
		}
	}

	// [地面兜底]
	if (XMVectorGetY(position) < 0.5f) {
		position = XMVectorSetY(position, 0.5f);
		velocity = XMVectorSetY(velocity, 0.0f);
		g_IsJump = false;
	}

	// ==============================================================================
	// 4. 最终状态保存
	// ==============================================================================
	XMStoreFloat3(&g_PlayerPosition, position);
	XMStoreFloat3(&g_PlayerVelocity, velocity);
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