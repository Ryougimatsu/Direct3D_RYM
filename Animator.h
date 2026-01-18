#pragma once
#include <vector>
#include <string>
#include <map>
#include <DirectXMath.h>
#include "Skeleton.h"
#include "Animation.h"

class Animator
{
public:
    // ==========================================
    // 1. 生命周期与核心循环
    // ==========================================
    Animator();

    // 更新动画状态 (步进时间、处理混合)
    void Update(double deltaTime);

    // ==========================================
    // 2. 动画控制 (Control)
    // ==========================================
    // 播放接口：fadeTime 为过渡时长（秒）
    void PlayAnimation(const Animation* pNewAnim, bool loop = true, float fadeTime = 0.3f);

    // 设置播放速度倍率
    void SetSpeedScale(float scale) { m_SpeedScale = scale; }

    // ==========================================
    // 3. 矩阵数据获取 (Data Access)
    // ==========================================
    // 获取单个骨骼的全局变换矩阵
    DirectX::XMMATRIX GetBoneGlobalMatrix(int boneIndex) const;

    // 生成最终矩阵数组（主入口，用于提交给 Shader）
    std::vector<DirectX::XMMATRIX> GetFinalBoneMatrices(const Skeleton& skeleton);

    // 缓存的全局矩阵 (公开变量，部分逻辑可能直接访问)
    std::vector<DirectX::XMMATRIX> m_CachedGlobalMatrices;

    // ==========================================
    // 4. 状态查询 (State Query)
    // ==========================================
    // 判断当前是否正在播放某个指定动画
    bool IsPlaying(const Animation* anim) const {
        return mCurrentAnim == anim;
    }

    // 获取当前动画进度 (0.0 到 1.0)
    float GetCurrentAnimationProgress() const {
        if (!mCurrentAnim || mCurrentAnim->duration <= 0.0) return 0.0f;
        return (float)(mCurrentTime / mCurrentAnim->duration);
    }

private:
    // ==========================================
    // 核心计算函数 (Core Calculation)
    // ==========================================
    /**
     * @brief 负责采样、混合以及递归计算骨骼的全局矩阵
     * @param boneIndex 当前计算的骨骼索引
     * @param skeleton 骨骼层级数据
     * @param parentMatrix 父骨骼的全局动画矩阵
     * @return 当前骨骼的模型空间全局动画矩阵
     */
    DirectX::XMMATRIX CalculateBone(
        int boneIndex,
        const Skeleton& skeleton,
        const DirectX::XMMATRIX& parentMatrix
    );

    // ==========================================
    // 采样与插值 (Sampling & Interpolation)
    // ==========================================
    DirectX::XMVECTOR SamplePosition(const AnimationChannel& ch, double t);
    DirectX::XMVECTOR SampleRotation(const AnimationChannel& ch, double t);
    DirectX::XMVECTOR SampleScale(const AnimationChannel& ch, double t);

    // 查找关键帧索引的模板函数
    template<typename T>
    int FindKeyIndex(const std::vector<std::pair<double, T>>& keys, double t);

    // ==========================================
    // 成员变量 (Member Variables)
    // ==========================================
    // 当前动画状态
    const Animation* mCurrentAnim;
    double           mCurrentTime;
    bool             mLoop;

    // 过渡状态（用于平滑切换动作）
    const Animation* mOldAnim;
    double           mOldTime;
    float            mFadeTime;   // 过渡总时长
    float            mFadeFactor; // 0.0=旧, 1.0=新

    // 通用设置
    float            m_SpeedScale = 1.0f;
};