#include "sprite_anime.h"
#include <DirectXMath.h>
#include "bullet_hit_effect.h"
using namespace DirectX;
#include "texture.h"

namespace {
	int g_TexID = -1;
	int g_AnimePatternID = -1;
}


class BulletHitEffect
{
private:
	XMFLOAT3 m_position{};      // 特效发生的位置
	int m_AnimePlayId{ -1 };    // 动画播放ID
	bool m_is_destroy{ false }; // 标记特效是否播放完毕，可以销毁

public:
	// 构造函数：初始化特效位置
	BulletHitEffect(const XMFLOAT3& position)
		: m_position(position), m_AnimePlayId(SpriteAnime_CreatePlayer(g_AnimePatternID)){
		
	}
	~BulletHitEffect(){
		SpriteAnime_DestroyPlayer(m_AnimePlayId);

	}

	void Update();      // 更新逻辑（例如判断动画是否播放完）
	void Draw() const;  // 绘制逻辑（调用公告板绘制）

	// 判断是否可以销毁
	bool IsDestroy() const {
		return m_is_destroy; // 注意：图片中这里是空的，需要加上 return
	}
};

void BulletHitEffect::Update()
{
	if (SpriteAnime_IsStopped(m_AnimePlayId))
	{
		m_is_destroy = true;
	}
}

void BulletHitEffect::Draw() const
{
	BillboardAnim_Draw(m_AnimePlayId,
		m_position,
		{ 1.0f,1.0f });
}

static constexpr int MAX_BULLET_HIT_EFFECT = 256;
static BulletHitEffect* g_pEffects[MAX_BULLET_HIT_EFFECT]{ nullptr };
static int g_EffectCount = 0;

void BulletHitEffect_Initialize()
{
	g_TexID = Texture_LoadFromFile(L"resource/texture/pl00.png");
	g_AnimePatternID = SpriteAnime_PatternRegister(
		g_TexID,
		8,				//パターン数
		0.3,			//1パターンあたりの秒数
		{ 32,49 },		//パターンサイズ
		{ 0,0 },			//スタート座標
		false,			//ループ設定
		8				//パターンの列数
	);
	g_EffectCount = 0;
}

void BulletHitEffect_Finalize()
{
	for (int i = 0; i < g_EffectCount; i++)
	{
		delete g_pEffects[i];
		g_pEffects[i] = nullptr;
	}
}

void BulletHitEffect_Update()
{
	for (int i = 0; i < g_EffectCount; i++)
	{
		g_pEffects[i]->Update();
	}
	for (int i = 0; i < g_EffectCount; i++)
	{
		if (g_pEffects[i]->IsDestroy())
		{
			delete g_pEffects[i];
			g_pEffects[i] = g_pEffects[g_EffectCount - 1];
			g_pEffects[g_EffectCount - 1] = nullptr;
			g_EffectCount--;
			i--;
		}
	}
}


void BulletHitEffect_Create(const DirectX::XMFLOAT3& position)
{
	g_pEffects[g_EffectCount] = new BulletHitEffect(position);
	g_EffectCount++;
}

void BulletHitEffect_Draw()
{
	for (int i = 0; i < g_EffectCount; i++)
	{
		g_pEffects[i]->Draw();
	}
}
