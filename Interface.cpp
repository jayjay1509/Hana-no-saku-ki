#include "Interface.hpp"
#include "Rendu.hpp"
#include "Settings.hpp"
#include "Simulation.hpp"

#include <imgui.h>
#include <tracy/Tracy.hpp>

bool simulationPaused = false;
bool stepSimulation = false;

void drawInterface()
{
    ZoneScopedN("drawInterface");

    ImGui::Begin("Simulation");

    ImGui::Checkbox("Pause", &simulationPaused);
    ImGui::SameLine();
    if (ImGui::Button("Step"))
        stepSimulation = true;

    ImGui::Checkbox("Pressure colors", &showPressure);
    ImGui::SliderInt("Brush size", &brushSize, 1, 8);
    ImGui::Checkbox("Circular brush", &settings.circularBrush);

    if (ImGui::Button("Reset"))
        resetGrid();

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
