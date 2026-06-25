#include "Benchmark.hpp"

#include "Interface.hpp"
#include "Simulation.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <thread>
#include <vector>

PerformanceStats performanceStats;
BenchmarkStats benchmarkStats;

namespace
{
    constexpr float benchmarkDurationSeconds = 10.0f;
    bool benchmarkRunRequested = false;

    struct BenchmarkAccumulator
    {
        bool active = false;
        std::chrono::steady_clock::time_point startTime;
        float frameMsTotal = 0.0f;
        float simulationMsTotal = 0.0f;
        float renderMsTotal = 0.0f;
        float interfaceMsTotal = 0.0f;
        float minFrameMs = std::numeric_limits<float>::max();
        float maxFrameMs = 0.0f;
        int frames = 0;
    };

    struct FrameSample
    {
        int frame = 0;
        float elapsedSeconds = 0.0f;
        float frameMs = 0.0f;
        float simulationMs = 0.0f;
        float renderMs = 0.0f;
        float interfaceMs = 0.0f;
        float fps = 0.0f;
    };

    BenchmarkAccumulator benchmarkAccumulator;
    std::filesystem::path benchmarkOutputDirectory;
    std::vector<FrameSample> benchmarkSamples;

    float elapsedSeconds(std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end)
    {
        return std::chrono::duration<float>(end - begin).count();
    }

    std::filesystem::path projectDirectory()
    {
        return std::filesystem::path(__FILE__).parent_path();
    }

    std::string timestamp()
    {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
        localtime_s(&localTime, &nowTime);

        std::ostringstream stream;
        stream << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S");
        return stream.str();
    }

    std::filesystem::path createBenchmarkDirectory()
    {
        std::filesystem::path directory = projectDirectory() / "benchmarks" / timestamp();
        std::filesystem::create_directories(directory);
        return directory;
    }

    std::string quoted(const std::filesystem::path &path)
    {
        return "\"" + path.string() + "\"";
    }

    std::string cmdCommand(const std::string &command)
    {
        return "cmd.exe /S /C \"" + command + "\"";
    }

    void startTracyCapture(const std::filesystem::path &outputDirectory)
    {
        const std::filesystem::path captureTool = projectDirectory() / "tracy" / "tracy-capture.exe";
        const std::filesystem::path exportTool = projectDirectory() / "tracy" / "tracy-csvexport.exe";
        const std::filesystem::path captureFile = outputDirectory / "benchmark_capture.tracy";
        const std::filesystem::path csvFile = outputDirectory / "benchmark_zones.csv";
        const std::filesystem::path logFile = outputDirectory / "tracy_log.txt";

        std::thread([](std::filesystem::path captureToolPath,
                       std::filesystem::path exportToolPath,
                       std::filesystem::path captureOutputPath,
                       std::filesystem::path csvOutputPath,
                       std::filesystem::path logOutputPath)
        {
            const std::string captureCommandCore =
                quoted(captureToolPath) + " -a 127.0.0.1 -p 8086 -o " + quoted(captureOutputPath) + " -f -s 10 >> " + quoted(logOutputPath) + " 2>&1";
            const std::string exportCommandCore =
                quoted(exportToolPath) + " " + quoted(captureOutputPath) + " > " + quoted(csvOutputPath) + " 2>> " + quoted(logOutputPath);
            const std::string captureCommand = cmdCommand(captureCommandCore);
            const std::string exportCommand = cmdCommand(exportCommandCore);

            {
                std::ofstream log(logOutputPath);
                log << "Tracy capture command:\n";
                log << captureCommandCore << "\n\n";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            int captureExitCode = 1;
            for (int attempt = 1; attempt <= 3; attempt++)
            {
                {
                    std::ofstream log(logOutputPath, std::ios::app);
                    log << "tracy-capture attempt " << attempt << " / 3\n";
                }

                captureExitCode = std::system(captureCommand.c_str());

                if (captureExitCode == 0 && std::filesystem::exists(captureOutputPath))
                    break;

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            {
                std::ofstream log(logOutputPath, std::ios::app);
                log << "\ntracy-capture exit code: " << captureExitCode << "\n";
            }

            if (!std::filesystem::exists(captureOutputPath))
            {
                std::ofstream log(logOutputPath, std::ios::app);
                log << "No benchmark_capture.tracy file was created, skipping tracy-csvexport.\n";
                log << "The application benchmark CSV files are still valid.\n";
                return;
            }

            {
                std::ofstream log(logOutputPath, std::ios::app);
                log << "\nTracy CSV export command:\n";
                log << exportCommandCore << "\n\n";
            }

            int exportExitCode = std::system(exportCommand.c_str());

            {
                std::ofstream log(logOutputPath, std::ios::app);
                log << "\ntracy-csvexport exit code: " << exportExitCode << "\n";
            }
        }, captureTool, exportTool, captureFile, csvFile, logFile).detach();
    }

    void saveBenchmarkResults(const BenchmarkStats &stats, const std::filesystem::path &outputDirectory)
    {
        GridStats gridStats = computeGridStats();
        std::ofstream file(outputDirectory / "benchmark_results.csv");

        file << "metric,value\n";
        file << "duration_seconds," << benchmarkDurationSeconds << "\n";
        file << "frames," << stats.frames << "\n";
        file << "average_frame_ms," << stats.avgFrameMs << "\n";
        file << "average_simulation_ms," << stats.avgSimulationMs << "\n";
        file << "average_render_ms," << stats.avgRenderMs << "\n";
        file << "average_interface_ms," << stats.avgInterfaceMs << "\n";
        file << "min_frame_ms," << stats.minFrameMs << "\n";
        file << "max_frame_ms," << stats.maxFrameMs << "\n";
        file << "water_cells," << gridStats.waterCells << "\n";
        file << "wall_cells," << gridStats.wallCells << "\n";
        file << "total_mass," << gridStats.totalMass << "\n";
    }

    void saveBenchmarkFrames(const std::filesystem::path &outputDirectory)
    {
        std::ofstream file(outputDirectory / "benchmark_frames.csv");

        file << "frame,elapsed_seconds,frame_ms,simulation_ms,render_ms,interface_ms,fps\n";

        for (const FrameSample &sample : benchmarkSamples)
        {
            file << sample.frame << ","
                 << sample.elapsedSeconds << ","
                 << sample.frameMs << ","
                 << sample.simulationMs << ","
                 << sample.renderMs << ","
                 << sample.interfaceMs << ","
                 << sample.fps << "\n";
        }
    }

    void saveBenchmarkInfo(const BenchmarkStats &stats, const std::filesystem::path &outputDirectory)
    {
        std::ofstream file(outputDirectory / "benchmark_info.txt");

        file << "Benchmark duration: " << benchmarkDurationSeconds << " seconds\n";
        file << "Output directory: " << stats.outputDirectory << "\n";
        file << "Expected files:\n";
        file << "- benchmark_results.csv: summary generated by the application\n";
        file << "- benchmark_frames.csv: one row per rendered frame\n";
        file << "- benchmark_info.txt: description of the benchmark output\n";
        file << "- tracy_log.txt: Tracy command output and exit codes\n";
        file << "- benchmark_capture.tracy: Tracy capture, only if tracy-capture connected successfully\n";
        file << "- benchmark_zones.csv: Tracy zone export, only if the capture file was produced\n";
    }
}

float elapsedMs(std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end)
{
    return std::chrono::duration<float, std::milli>(end - begin).count();
}

void requestBenchmarkRun()
{
    benchmarkRunRequested = true;
}

void startBenchmarkIfRequested(std::chrono::steady_clock::time_point frameStart)
{
    if (!benchmarkRunRequested || benchmarkAccumulator.active)
        return;

    benchmarkRunRequested = false;
    loadBenchmarkScene();
    simulationPaused = false;

    benchmarkStats = BenchmarkStats{};
    benchmarkStats.running = true;
    benchmarkOutputDirectory = createBenchmarkDirectory();
    benchmarkStats.outputDirectory = benchmarkOutputDirectory.string();
    benchmarkStats.tracyStatus = "capture running in background; check tracy_log.txt after a few seconds";

    benchmarkAccumulator = BenchmarkAccumulator{};
    benchmarkAccumulator.active = true;
    benchmarkAccumulator.startTime = frameStart;
    benchmarkSamples.clear();
    benchmarkSamples.reserve(700);

    startTracyCapture(benchmarkOutputDirectory);
}

void updatePerformanceStats(
    std::chrono::steady_clock::time_point frameStart,
    std::chrono::steady_clock::time_point frameEnd,
    std::chrono::steady_clock::time_point simulationStart,
    std::chrono::steady_clock::time_point simulationEnd,
    std::chrono::steady_clock::time_point renderStart,
    std::chrono::steady_clock::time_point renderEnd,
    std::chrono::steady_clock::time_point interfaceStart,
    std::chrono::steady_clock::time_point interfaceEnd)
{
    performanceStats.simulationMs = elapsedMs(simulationStart, simulationEnd);
    performanceStats.renderMs = elapsedMs(renderStart, renderEnd);
    performanceStats.interfaceMs = elapsedMs(interfaceStart, interfaceEnd);
    performanceStats.frameMs = elapsedMs(frameStart, frameEnd);
    performanceStats.fps = performanceStats.frameMs > 0.0f ? 1000.0f / performanceStats.frameMs : 0.0f;
}

void recordBenchmarkFrame(std::chrono::steady_clock::time_point frameEnd)
{
    if (!benchmarkAccumulator.active)
        return;

    benchmarkAccumulator.frames++;
    benchmarkAccumulator.frameMsTotal += performanceStats.frameMs;
    benchmarkAccumulator.simulationMsTotal += performanceStats.simulationMs;
    benchmarkAccumulator.renderMsTotal += performanceStats.renderMs;
    benchmarkAccumulator.interfaceMsTotal += performanceStats.interfaceMs;
    benchmarkAccumulator.minFrameMs = std::min(benchmarkAccumulator.minFrameMs, performanceStats.frameMs);
    benchmarkAccumulator.maxFrameMs = std::max(benchmarkAccumulator.maxFrameMs, performanceStats.frameMs);

    benchmarkStats.elapsedSeconds = elapsedSeconds(benchmarkAccumulator.startTime, frameEnd);
    benchmarkSamples.push_back(FrameSample{
        benchmarkAccumulator.frames,
        benchmarkStats.elapsedSeconds,
        performanceStats.frameMs,
        performanceStats.simulationMs,
        performanceStats.renderMs,
        performanceStats.interfaceMs,
        performanceStats.fps
    });

    if (benchmarkStats.elapsedSeconds < benchmarkDurationSeconds)
        return;

    benchmarkStats.running = false;
    benchmarkStats.finished = true;
    benchmarkStats.frames = benchmarkAccumulator.frames;
    benchmarkStats.avgFrameMs = benchmarkAccumulator.frameMsTotal / benchmarkAccumulator.frames;
    benchmarkStats.avgSimulationMs = benchmarkAccumulator.simulationMsTotal / benchmarkAccumulator.frames;
    benchmarkStats.avgRenderMs = benchmarkAccumulator.renderMsTotal / benchmarkAccumulator.frames;
    benchmarkStats.avgInterfaceMs = benchmarkAccumulator.interfaceMsTotal / benchmarkAccumulator.frames;
    benchmarkStats.minFrameMs = benchmarkAccumulator.minFrameMs;
    benchmarkStats.maxFrameMs = benchmarkAccumulator.maxFrameMs;
    benchmarkStats.tracyStatus = "capture/export may finish shortly; see tracy_log.txt";

    saveBenchmarkResults(benchmarkStats, benchmarkOutputDirectory);
    saveBenchmarkFrames(benchmarkOutputDirectory);
    saveBenchmarkInfo(benchmarkStats, benchmarkOutputDirectory);
    benchmarkAccumulator.active = false;
}
