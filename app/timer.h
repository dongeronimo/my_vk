#pragma once

namespace app {
    class Timer
    {
    public:
        Timer();
        void Advance();
        const float DeltaTime()const {
            return mDeltaTime;
        }
        const float SecondsSinceStart()const {
            return mSecondsSinceStart;
        }
    private:
        float mDeltaTime, mSecondsSinceStart;
    };
}