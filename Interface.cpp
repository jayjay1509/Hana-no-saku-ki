#include "Interface.hpp"
#include "Benchmark.hpp"
#include "Rendu.hpp"
#include "Settings.hpp"
#include "Simulation.hpp"

#include <imgui.h>
#include <tracy/Tracy.hpp>

bool simulationPaused = false;
bool stepSimulation = false;
bool limitFramerate = true;
int targetFramerate = 60;
bool fullscreenEnabled = false;

void drawInterface()
{
    ZoneScopedN("drawInterface");

    ImGui::Begin("Simulation");

    ImGui::Checkbox("Pause", &simulationPaused);
    ImGui::SameLine();
    if (ImGui::Button("Step"))
        stepSimulation = true;

    ImGui::Checkbox("Pressure colors", &showPressure);
    ImGui::Checkbox("Fullscreen", &fullscreenEnabled);
    ImGui::Checkbox("Limit FPS", &limitFramerate);
    ImGui::BeginDisabled(!limitFramerate);
    ImGui::SliderInt("FPS cap", &targetFramerate, 30, 240);
    ImGui::EndDisabled();
    ImGui::SliderInt("Brush size", &brushSize, 1, 8);
    ImGui::Checkbox("Circular brush", &settings.circularBrush);

    if (ImGui::Button("Reset"))
        resetGrid();
    ImGui::SameLine();
    if (ImGui::Button("Benchmark scene"))
        loadBenchmarkScene();
    ImGui::SameLine();
    ImGui::BeginDisabled(benchmarkStats.running);
    if (ImGui::Button("Run 10s benchmark"))
        requestBenchmarkRun();
    ImGui::EndDisabled();

    if (ImGui::CollapsingHeader("Measurements", ImGuiTreeNodeFlags_DefaultOpen))
    {
        GridStats stats = computeGridStats();

        ImGui::Text("FPS: %.1f", performanceStats.fps);
        ImGui::Text("Frame: %.3f ms", performanceStats.frameMs);
        ImGui::Text("Simulation: %.3f ms", performanceStats.simulationMs);
        ImGui::Text("Render: %.3f ms", performanceStats.renderMs);
        ImGui::Text("Interface: %.3f ms", performanceStats.interfaceMs);
        ImGui::Separator();
        ImGui::Text("Water cells: %d", stats.waterCells);
        ImGui::Text("Wall cells: %d", stats.wallCells);
        ImGui::Text("Total mass: %.3f", stats.totalMass);
        ImGui::Separator();

        if (benchmarkStats.running)
        {
            ImGui::Text("Benchmark: running %.1f / 10.0 s", benchmarkStats.elapsedSeconds);
            ImGui::TextWrapped("Tracy: %s", benchmarkStats.tracyStatus.c_str());
        }
        else if (benchmarkStats.finished)
        {
            ImGui::Text("Benchmark: finished");
            ImGui::Text("Frames: %d", benchmarkStats.frames);
            ImGui::Text("Average frame: %.3f ms", benchmarkStats.avgFrameMs);
            ImGui::Text("Average simulation: %.3f ms", benchmarkStats.avgSimulationMs);
            ImGui::Text("Average render: %.3f ms", benchmarkStats.avgRenderMs);
            ImGui::Text("Average interface: %.3f ms", benchmarkStats.avgInterfaceMs);
            ImGui::Text("Min / max frame: %.3f / %.3f ms", benchmarkStats.minFrameMs, benchmarkStats.maxFrameMs);
            ImGui::TextWrapped("Tracy: %s", benchmarkStats.tracyStatus.c_str());
            ImGui::TextWrapped("Folder: %s", benchmarkStats.outputDirectory.c_str());
        }
    }

    if (ImGui::CollapsingHeader("Simulation tuning", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Limit flow", &settings.useMaxFlow);
        ImGui::BeginDisabled(!settings.useMaxFlow);
        ImGui::SliderFloat("Max flow", &settings.maxFlow, 0.01f, 1.0f, "%.2f");
        ImGui::EndDisabled();

        ImGui::Checkbox("Viscosity", &settings.useViscosity);
        ImGui::BeginDisabled(!settings.useViscosity);
        ImGui::SliderFloat("Viscosity value", &settings.viscosity, 0.05f, 1.0f, "%.2f");
        ImGui::EndDisabled();

        ImGui::Checkbox("Compressibility", &settings.useCompressibility);
        ImGui::BeginDisabled(!settings.useCompressibility);
        ImGui::SliderFloat("Compression", &settings.compressibility, 0.0f, 1.5f, "%.2f");
        ImGui::EndDisabled();

        ImGui::Checkbox("Alternate update direction", &settings.alternateUpdateDirection);
        ImGui::Checkbox("Stable water noise", &settings.stableNoise);

        if (ImGui::Button("Reset tuning"))
            resetSettings();
    }

    ImGui::Text("Left: water");
    ImGui::Text("Right: wall");
    ImGui::Text("Middle: erase");

    ImGui::End();
}
