#pragma once
#include "Resource/AnimationClip.h"

enum class PlaybackMode
{
    Loop,
    Once,
    PingPong
};

enum class LayerBlendMode
{
    Override,
    Additive
};

struct AnimationState
{
    std::shared_ptr<AnimationClip> clip;
    float currentTime = 0.0f;
    float speed = 1.0f;
    float weight = 1.0f;

    PlaybackMode mode = PlaybackMode::Loop;
    bool isReverse = false;
    bool isValid = false;

    void Reset(std::shared_ptr<AnimationClip> newClip, float newSpeed, PlaybackMode newMode)
    {
        clip = newClip;
        speed = newSpeed;
        mode = newMode;
        currentTime = 0.0f;
        weight = 1.0f;
        isReverse = false;
        isValid = (clip != nullptr);
    }

    void Update(float deltaTime)
    {
        if (!isValid || !clip) return;

        float duration = clip->GetDuration();
        if (duration <= 0.0f) return;

        float actualSpeed = speed;
        if (mode == PlaybackMode::PingPong && isReverse)
            actualSpeed = -speed;

        currentTime += deltaTime * actualSpeed;

        if (mode == PlaybackMode::Loop)
        {
            if (currentTime >= duration)
                currentTime = fmod(currentTime, duration);
            else if (currentTime < 0.0f)
                currentTime = duration + fmod(currentTime, duration);
        }
        else if (mode == PlaybackMode::Once)
        {
            if (currentTime >= duration) currentTime = duration;
            else if (currentTime <= 0.0f) currentTime = 0.0f;
        }
        else if (mode == PlaybackMode::PingPong)
        {
            if (currentTime >= duration)
            {
                currentTime = duration;
                isReverse = true;
            }
            else if (currentTime <= 0.0f)
            {
                currentTime = 0.0f;
                isReverse = false;
            }
        }
    }
};