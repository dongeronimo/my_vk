#include "timer.h"
#include <chrono>
static auto startTime = std::chrono::high_resolution_clock::now();
static auto lastFrameTime = std::chrono::high_resolution_clock::now();
namespace app
{
    Timer::Timer() {
        
    }
    void Timer::Advance()
    {
        static auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        mSecondsSinceStart = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        mDeltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
    }
}