#include <SFML/Graphics.hpp>
#include <imgui.h>
#include "imgui-SFML.h"
#include <tracy/Tracy.hpp>
#include "Interface.hpp"
#include "Simulation.hpp"
#include "Rendu.hpp"

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({ hauteur * cellule_taille, largeur * cellule_taille }),
        "Fluid Simulation - Walls + Pressure"
    );

    window.setFramerateLimit(60);

    sf::RectangleShape cell({ cellule_taille - 1.f, cellule_taille - 1.f });
    sf::Clock deltaClock;

    if (!window.setActive(true))
        return 1;

    if (!ImGui::SFML::Init(window))
        return 1;

    ImGui::GetIO().IniFilename = nullptr;

    while (window.isOpen())
    {
        handleEvents(window);
        ImGui::SFML::Update(window, deltaClock.restart());
        handleInput(window);

        if (!simulationPaused || stepSimulation)
        {
            updateFluid();
            stepSimulation = false;
        }

        drawGrid(window, cell);
        drawInterface();
        ImGui::SFML::Render(window);
        window.display();
        FrameMark;
    }

    ImGui::SFML::Shutdown();
}
