#pragma once

#include <chrono>
#include <string>

struct PerformanceStats
{
    float frameMs = 0.0f;
    float simulationMs = 0.0f;
    float renderMs = 0.0f;
    float interfaceMs = 0.0f;
    float fps = 0.0f;
};

struct BenchmarkStats
{
    bool running = false;
    bool finished = false;
    float elapsedSeconds = 0.0f;
    int frames = 0;
    float avgFrameMs = 0.0f;
    float avgSimulationMs = 0.0f;
    float avgRenderMs = 0.0f;
    float avgInterfaceMs = 0.0f;
    float minFrameMs = 0.0f;
    float maxFrameMs = 0.0f;
    std::string outputDirectory;
    std::string tracyStatus;
};

extern PerformanceStats performanceStats;
extern BenchmarkStats benchmarkStats;

float elapsedMs(std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end);
void requestBenchmarkRun();
void startBenchmarkIfRequested(std::chrono::steady_clock::time_point frameStart);
void updatePerformanceStats(
    std::chrono::steady_clock::time_point frameStart,
    std::chrono::steady_clock::time_point frameEnd,
    std::chrono::steady_clock::time_point simulationStart,
    std::chrono::steady_clock::time_point simulationEnd,
    std::chrono::steady_clock::time_point renderStart,
    std::chrono::steady_clock::time_point renderEnd,
    std::chrono::steady_clock::time_point interfaceStart,
    std::chrono::steady_clock::time_point interfaceEnd);
void recordBenchmarkFrame(std::chrono::steady_clock::time_point frameEnd);
