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
        return sf::VideoMode({ hauteur * cellule_taille, largeur * cellule_taille });
    }

    void applyFramerateLimit(sf::RenderWindow &window)
    {
        window.setFramerateLimit(limitFramerate ? targetFramerate : 0);
    }

    bool recreateWindow(sf::RenderWindow &window)
    {
        ImGui::SFML::Shutdown();

        if (fullscreenEnabled)
        {
            window.create(
                sf::VideoMode::getDesktopMode(),
                "Fluid Simulation - Walls + Pressure",
                sf::Style::Default,
                sf::State::Fullscreen
            );
        }
        else
        {
            window.create(
                windowedMode(),
                "Fluid Simulation - Walls + Pressure",
                sf::Style::Default,
                sf::State::Windowed
            );
        }

        if (!window.setActive(true))
            return false;

        applyFramerateLimit(window);

        if (!ImGui::SFML::Init(window))
            return false;

        ImGui::GetIO().IniFilename = nullptr;
        return true;
    }
}

int main()
{
    sf::RenderWindow window(
        windowedMode(),
        "Fluid Simulation - Walls + Pressure"
    );

    applyFramerateLimit(window);
    bool appliedLimitFramerate = limitFramerate;
    int appliedTargetFramerate = targetFramerate;
    bool appliedFullscreen = fullscreenEnabled;

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

        if (appliedFullscreen != fullscreenEnabled)
        {
            if (!recreateWindow(window))
                return 1;

            appliedFullscreen = fullscreenEnabled;
            appliedLimitFramerate = limitFramerate;
            appliedTargetFramerate = targetFramerate;
            deltaClock.restart();
        }

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
