#pragma once
#include <chrono>

namespace Henky3D {

class Timer {
public:
    Timer() { Reset(); }

    void Reset() {
        m_StartTime = std::chrono::high_resolution_clock::now();
        m_LastTime = m_StartTime;
    }

    float GetDeltaTime() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - m_LastTime).count();
        m_LastTime = currentTime;
        return deltaTime;
    }

    float GetElapsedTime() const {
        auto currentTime = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(currentTime - m_StartTime).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_LastTime;
};

class FPSCounter {
public:
    void Update(float deltaTime) {
        m_FrameCount++;
        m_AccumulatedTime += deltaTime;
        
        if (m_AccumulatedTime >= 1.0f) {
            m_FPS = m_FrameCount / m_AccumulatedTime;
            m_FrameTime = m_AccumulatedTime / m_FrameCount * 1000.0f;
            m_FrameCount = 0;
            m_AccumulatedTime = 0.0f;
        }
    }

    float GetFPS() const { return m_FPS; }
    float GetFrameTime() const { return m_FrameTime; }

private:
    int m_FrameCount = 0;
    float m_AccumulatedTime = 0.0f;
    float m_FPS = 0.0f;
    float m_FrameTime = 0.0f;
};

} // namespace Henky3D
