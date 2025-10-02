#pragma once

class GameTimer
{
public:
    GameTimer();

    void Tick(float lockFPS = 0.0f);
    void Start();
    void Stop();
    void Reset();

    unsigned long GetFrameRate();
    float GetDeltaTime() const { return (float)mDeltaTime; }
    float GetRunTime() const;
    void  SetRunTime(float seconds);

private:
    double      mSecondsPerCount = 0.0;
    double      mDeltaTime = 0.0;

    __int64     mBaseTime = 0;
    __int64     mPausedTime = 0;
    __int64     mStopTime = 0;
    __int64     mPrevTime = 0;
    __int64     mCurrTime = 0;

    bool        mStopped = false;

    // FPS 
    unsigned int mFrameCount = 0;
    unsigned int mFPS = 0;
    double       mFPSTimeElapsed = 0.0;
};
