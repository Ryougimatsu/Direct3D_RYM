#include "sprite_anime.h"
#include "sprite.h"
#include "texture.h"
#include <DirectXMath.h>
#include "billboard.h"
#include <vector>
using namespace DirectX;

struct AnimePatternData {
	int m_TextureId = -1;// テクスチャID
	int m_PatternMax = 0;//パ夕-ン数
	int m_PatternCol = 1;//パターンの列数（横方向）
	XMUINT2 m_StartPosition {0,0};//ア二メ一ションのスタ一卜座標
	XMUINT2 m_PatternSize = { 0,0 };//アニメーションパターンのサイズ
	bool m_IsLooped = true;//ル-プするか
	double m_second_per_pattern = 0.1; // パターンごとの秒数（デフォルトは0.1秒）

	bool m_IsSequenceMode = false;
	std::vector<int> m_SequenceTextureIds;
};


struct AnimePlayData {
	int m_PatternId = -1;//アニメ-ションパタ-ンID
	int m_PatternNum = 0;//現在再生中のパ夕一ン番号
	double m_AccumulatedTime = 0.0;// 累積時間
	bool m_isStopped = false; // 再生が停止しているかどうか
};

static constexpr int ANIM_PATTERN_MAX = 128;
static AnimePatternData g_AnimePattern[ANIM_PATTERN_MAX]; // アニメーションパターンデータ
static constexpr int ANIM_PLAY_MAX = 256;
static AnimePlayData g_AnimePlayData[ANIM_PLAY_MAX]; // アニメーション再生データ




void SpriteAnime_Initialize()
{
	for (AnimePatternData& data : g_AnimePattern) {
		data.m_TextureId = -1; // 初期化
		data.m_IsSequenceMode = false;     // [新增]
		data.m_SequenceTextureIds.clear(); // [新增] 清空序列

	}

	for (AnimePlayData& data : g_AnimePlayData) {
		data.m_PatternId = -1; // 初期化
		data.m_isStopped = false; // 再生が停止しているかどうかを初期化
	}
}

void SpriteAnime_Finalize(void)
{
}

void SpriteAnime_Update(double elapsed_time)
{
	for (int i = 0; i < ANIM_PLAY_MAX; i++) {
		if (g_AnimePlayData[i].m_PatternId < 0) continue; // 使用されていないプレイヤーはスキップ
		AnimePatternData* pAnmPtrnData = &g_AnimePattern[g_AnimePlayData[i].m_PatternId];

		if (g_AnimePlayData[i].m_AccumulatedTime >= pAnmPtrnData->m_second_per_pattern) {

			g_AnimePlayData[i].m_PatternNum++; // パターン番号を更新
			
			if (g_AnimePlayData[i].m_PatternNum >= pAnmPtrnData->m_PatternMax) {

				if (pAnmPtrnData->m_IsLooped) {

					g_AnimePlayData[i].m_PatternNum = 0; // ループする場合はパターン番号をリセット

				}
				else {

					g_AnimePlayData[i].m_PatternNum = pAnmPtrnData->m_PatternMax - 1; // ループしない場合は最後のパターンに留める
					g_AnimePlayData[i].m_isStopped = true; // 再生を停止
				}
				
			}
			g_AnimePlayData[i].m_AccumulatedTime -= pAnmPtrnData->m_second_per_pattern; // 累積時間から経過時間を引く
			
		}
		g_AnimePlayData[i].m_AccumulatedTime += elapsed_time; // 経過時間を更新
	}
}
	void SpriteAnime_Draw(int playid, float x, float y, float dw, float dh) {

		int anm_ptrn_id = g_AnimePlayData[playid].m_PatternId;

		AnimePatternData* pAnmPtrnData = &g_AnimePattern[anm_ptrn_id];

		Sprite_Draw(pAnmPtrnData->m_TextureId,
			x, y, dw, dh,
			pAnmPtrnData->m_StartPosition.x + pAnmPtrnData->m_PatternSize.x 
			* (g_AnimePlayData[playid].m_PatternNum % pAnmPtrnData->m_PatternCol),
			pAnmPtrnData->m_StartPosition.y + pAnmPtrnData->m_PatternSize.y 
			* (g_AnimePlayData[playid].m_PatternNum / pAnmPtrnData->m_PatternCol),
			pAnmPtrnData->m_PatternSize.x,
			pAnmPtrnData->m_PatternSize.y);

	}

	void BillboardAnim_Draw(int playid, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot)
	{
		int anm_ptrn_id = g_AnimePlayData[playid].m_PatternId;

		AnimePatternData* pAnmPtrnData = &g_AnimePattern[anm_ptrn_id];

		Billboard_Draw(
			pAnmPtrnData->m_TextureId,
			position,
			scale,
			{
				(float)pAnmPtrnData->m_StartPosition.x + pAnmPtrnData->m_PatternSize.x * (g_AnimePlayData[playid].m_PatternNum % pAnmPtrnData->m_PatternCol),
				(float)pAnmPtrnData->m_StartPosition.y + pAnmPtrnData->m_PatternSize.y * (g_AnimePlayData[playid].m_PatternNum / pAnmPtrnData->m_PatternCol),
				(float)pAnmPtrnData->m_PatternSize.x,
				(float)pAnmPtrnData->m_PatternSize.y
			},
			pivot
		);

	}

	int SpriteAnime_PatternRegister(int textrueId, int pattern_max, double second_per_pattern,
		const DirectX::XMUINT2& pattern_size, const DirectX::XMUINT2& start_position, bool isLooped, int pattern_col)
	{
		for (int i = 0; i < ANIM_PATTERN_MAX; i++) {

			if (g_AnimePattern[i].m_TextureId >= 0)continue; // 既に使用中のパターンはスキップ

			g_AnimePattern[i].m_TextureId = textrueId; // テクスチャIDを設定
			g_AnimePattern[i].m_PatternMax = pattern_max; // パターン数を設定
			g_AnimePattern[i].m_second_per_pattern = second_per_pattern; // パターンごとの秒数を設定
			g_AnimePattern[i].m_PatternCol = pattern_col; // パターンの列数を設定
			g_AnimePattern[i].m_StartPosition = start_position; // アニメーションのスタート座標を設定
			g_AnimePattern[i].m_PatternSize = pattern_size; // アニメーションパターンのサイズを設定
			g_AnimePattern[i].m_IsLooped = isLooped; // ループ設定を保存
			return i; // 登録したパターンのIDを返す
		}
		return -1;
	}

	int SpriteAnime_CreatePlayer(int anime_pattern_id)
	{
		for (int i = 0; i < ANIM_PLAY_MAX; i++) {
			if (g_AnimePlayData[i].m_PatternId >= 0)continue; // 既に使用中のプレイヤーはスキップ
			g_AnimePlayData[i].m_PatternId = anime_pattern_id; // アニメーションパターンIDを設定
			g_AnimePlayData[i].m_PatternNum = 0; // パターン番号を初期化
			g_AnimePlayData[i].m_AccumulatedTime = 0.0; // 累積時間を初期化
			g_AnimePlayData[i].m_isStopped = false; // 再生が停止しているかどうかを初期化
			return i; // 登録したプレイヤーのIDを返す
		}	
		return -1;
	}

	int SpriteAnime_PatternRegisterSequence(const int* textureIds, int count, double second_per_pattern, bool isLooped)
	{
		for (int i = 0; i < ANIM_PATTERN_MAX; i++) {
			// 找一个没被占用的槽位 (这里判断稍微粗糙点，只要单图ID和序列都为空就视为可用)
			if (g_AnimePattern[i].m_TextureId == -1 && g_AnimePattern[i].m_SequenceTextureIds.empty()) {

				AnimePatternData& data = g_AnimePattern[i];

				data.m_IsSequenceMode = true; // 标记为序列模式
				data.m_PatternMax = count;
				data.m_second_per_pattern = second_per_pattern;
				data.m_IsLooped = isLooped;

				// 将传入的 ID 数组拷贝到 vector 中
				data.m_SequenceTextureIds.resize(count);
				for (int frame = 0; frame < count; ++frame) {
					data.m_SequenceTextureIds[frame] = textureIds[frame];
				}

				return i; // 返回 Pattern ID
			}
		}
		return -1; // 注册失败
	}

	bool SpriteAnime_IsStopped(int index)
	{
		return g_AnimePlayData[index].m_isStopped; // 再生が停止しているかどうかを返す
	}

	void SpriteAnime_FlipbookDraw(int playid, float x, float y, float dw, float dh)
	{
		// 安全检查
		if (playid < 0 || playid >= ANIM_PLAY_MAX) return;
		int anm_ptrn_id = g_AnimePlayData[playid].m_PatternId;
		if (anm_ptrn_id < 0) return;

		AnimePatternData* pAnmPtrnData = &g_AnimePattern[anm_ptrn_id];
		int currentFrame = g_AnimePlayData[playid].m_PatternNum;

		// --- 分支判断 ---
		if (pAnmPtrnData->m_IsSequenceMode) {
			// [新增] 多图序列模式逻辑
			if (currentFrame < pAnmPtrnData->m_SequenceTextureIds.size()) {
				int currentTexId = pAnmPtrnData->m_SequenceTextureIds[currentFrame];

				// 直接绘制整张图片 (UV 0,0 到 1,1)
				// 使用 sprite.h 中支持缩放的重载版本
				Sprite_Draw(currentTexId, x, y, dw, dh, { 1.0f, 1.0f, 1.0f, 1.0f });
			}
		}
		else {
			// [原有] Sprite Sheet 切分逻辑保持不变
			Sprite_Draw(pAnmPtrnData->m_TextureId,
				x, y, dw, dh,
				pAnmPtrnData->m_StartPosition.x + pAnmPtrnData->m_PatternSize.x
				* (currentFrame % pAnmPtrnData->m_PatternCol),
				pAnmPtrnData->m_StartPosition.y + pAnmPtrnData->m_PatternSize.y
				* (currentFrame / pAnmPtrnData->m_PatternCol),
				pAnmPtrnData->m_PatternSize.x,
				pAnmPtrnData->m_PatternSize.y);
		}
	}

	void SpriteAnime_DestroyPlayer(int index)
	{
		g_AnimePlayData[index].m_PatternId = -1; // アニメーションパターンIDを無効化
	}

