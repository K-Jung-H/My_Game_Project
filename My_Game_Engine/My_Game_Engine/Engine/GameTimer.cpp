#include "GameTimer.h"

GameTimer::GameTimer()
{
    __int64 countsPerSec = 0;
    ::QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    mSecondsPerCount = 1.0 / (double)countsPerSec;

    ::QueryPerformanceCounter((LARGE_INTEGER*)&mPrevTime);
    mBaseTime = mPrevTime;
}

void GameTimer::Tick(float lockFPS)
{
    if (mStopped)
    {
        mDeltaTime = 0.0;
        return;
    }

    ::QueryPerformanceCounter((LARGE_INTEGER*)&mCurrTime);

    mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

    if (lockFPS > 0.0f)
    {
        const double minFrameTime = 1.0 / lockFPS;
        while (mDeltaTime < minFrameTime)
        {
            ::QueryPerformanceCounter((LARGE_INTEGER*)&mCurrTime);
            mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
        }
    }

    mPrevTime = mCurrTime;

    // FPS 
    mFrameCount++;
    mFPSTimeElapsed += mDeltaTime;
    if (mFPSTimeElapsed >= 1.0)
    {
        mFPS = mFrameCount;
        mFrameCount = 0;
        mFPSTimeElapsed = 0.0;
    }
}

unsigned long GameTimer::GetFrameRate()
{
    return mFPS;
}

float GameTimer::GetRunTime() const
{
    if (mStopped)
        return (float)((mStopTime - mPausedTime - mBaseTime) * mSecondsPerCount);
    else
        return (float)((mCurrTime - mPausedTime - mBaseTime) * mSecondsPerCount);
}

void GameTimer::SetRunTime(float seconds)
{
    __int64 currTime = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    mBaseTime = currTime - (__int64)(seconds / mSecondsPerCount) - mPausedTime;

    if (!mStopped)
        mPrevTime = currTime;
}

void GameTimer::Reset()
{
    __int64 currTime = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    mBaseTime = currTime;
    mPrevTime = currTime;
    mStopTime = 0;
    mPausedTime = 0;
    mStopped = false;
}

void GameTimer::Start()
{
    if (mStopped)
    {
        __int64 startTime = 0;
        ::QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

        mPausedTime += (startTime - mStopTime);
        mPrevTime = startTime;
        mStopTime = 0;
        mStopped = false;
    }
}

void GameTimer::Stop()
{
    if (!mStopped)
    {
        __int64 currTime = 0;
        ::QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        mStopTime = currTime;
        mStopped = true;
    }
}
