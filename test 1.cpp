#include <SFML/Graphics.hpp>
#include <imgui.h>
#include "imgui-SFML.h"
#include <tracy/Tracy.hpp>
#include <chrono>
#include "Benchmark.hpp"
#include "Interface.hpp"
#include "Simulation.hpp"
#include "Rendu.hpp"

namespace
{
    sf::VideoMode windowedMode()
    {
        return sf::VideoMode({
            static_cast<unsigned int>(hauteur * cellule_taille),
            static_cast<unsigned int>(largeur * cellule_taille)
        });
    }

    void applyFramerateLimit(sf::RenderWindow &window)
    {
        window.setFramerateLimit(limitFramerate ? targetFramerate : 0);
    }
}

int main()
{
    sf::RenderWindow window(
        windowedMode(),
        "Fluid Simulation - Hana-no-saku-ki "
    );

    applyFramerateLimit(window);
    bool appliedLimitFramerate = limitFramerate;
    int appliedTargetFramerate = targetFramerate;

    sf::Clock deltaClock;

    if (!window.setActive(true))
        return 1;

    if (!ImGui::SFML::Init(window))
        return 1;

    ImGui::GetIO().IniFilename = nullptr;

    while (window.isOpen())
    {
        auto frameStart = std::chrono::steady_clock::now();
        startBenchmarkIfRequested(frameStart);

        if (appliedLimitFramerate != limitFramerate || appliedTargetFramerate != targetFramerate)
        {
            applyFramerateLimit(window);
            appliedLimitFramerate = limitFramerate;
            appliedTargetFramerate = targetFramerate;
        }

        handleEvents(window);
        ImGui::SFML::Update(window, deltaClock.restart());
        handleInput(window);

        auto simulationStart = std::chrono::steady_clock::now();
        if (!simulationPaused || stepSimulation)
        {
            updateFluid();
            stepSimulation = false;
        }
        auto simulationEnd = std::chrono::steady_clock::now();

        auto renderStart = std::chrono::steady_clock::now();
        drawGrid(window);
        auto renderEnd = std::chrono::steady_clock::now();

        auto interfaceStart = std::chrono::steady_clock::now();
        drawInterface();
        ImGui::SFML::Render(window);
        auto interfaceEnd = std::chrono::steady_clock::now();

        window.display();
        auto frameEnd = std::chrono::steady_clock::now();

        updatePerformanceStats(frameStart, frameEnd, simulationStart, simulationEnd, renderStart, renderEnd, interfaceStart, interfaceEnd);
        recordBenchmarkFrame(frameEnd);

        FrameMark;
    }

    ImGui::SFML::Shutdown();
}
