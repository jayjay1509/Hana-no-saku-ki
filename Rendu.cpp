#include "Rendu.hpp"
#include "Simulation.hpp"
#include "Settings.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <imgui.h>
#include "imgui-SFML.h"
#include <map>
#include <tracy/Tracy.hpp>

bool showPressure = true;
int brushSize = 1;

bool keyPressedOnce(sf::Keyboard::Key key)
{
    static std::map<sf::Keyboard::Key, bool> state;

    bool pressed = sf::Keyboard::isKeyPressed(key);
    bool result = pressed && !state[key];

    state[key] = pressed;
    return result;
}

bool inBrush(int centerX, int centerY, int x, int y)
{
    if (!settings.circularBrush)
        return true;

    int dx = x - centerX;
    int dy = y - centerY;
    int radius = brushSize - 1;
    return dx * dx + dy * dy <= radius * radius;
}

void handleEvents(sf::RenderWindow &window)
{
    ZoneScopedN("handleEvents");

    while (auto event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            window.close();

        ImGui::SFML::ProcessEvent(window, *event);
    }
}

void handleInput(sf::RenderWindow &window)
{
    ZoneScopedN("handleInput");

    auto pos = sf::Mouse::getPosition(window);
    int x = pos.x / cellule_taille;
    int y = pos.y / cellule_taille;

    if (ImGui::GetIO().WantCaptureMouse)
        return;

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
        for (int yy = y - brushSize + 1; yy <= y + brushSize - 1; yy++)
        {
            for (int xx = x - brushSize + 1; xx <= x + brushSize - 1; xx++)
            {
                if (inBrush(x, y, xx, yy) && inBounds(xx, yy) && cellAt(xx, yy).type != WALL)
                {
                    cellAt(xx, yy).mass = MAX_MASS;
                    cellAt(xx, yy).type = WATER;
                }
            }
        }
    }

    if (keyPressedOnce(sf::Keyboard::Key::P))
        showPressure = !showPressure;

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
    {
        for (int yy = y - brushSize + 1; yy <= y + brushSize - 1; yy++)
        {
            for (int xx = x - brushSize + 1; xx <= x + brushSize - 1; xx++)
            {
                if (inBrush(x, y, xx, yy) && inBounds(xx, yy))
                {
                    cellAt(xx, yy).type = WALL;
                    cellAt(xx, yy).mass = 0.0f;
                }
            }
        }
    }


    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
    {
        for (int yy = y - brushSize + 1; yy <= y + brushSize - 1; yy++)
        {
            for (int xx = x - brushSize + 1; xx <= x + brushSize - 1; xx++)
            {
                if (inBrush(x, y, xx, yy) && inBounds(xx, yy))
                {
                    cellAt(xx, yy).type = EMPTY;
                    cellAt(xx, yy).mass = 0.0f;
                }
            }
        }
    }
}

sf::Color getWaterColor(int x, int y, Cell &c)
{
    if (showPressure)
    {
        float m = c.mass;

        float fill = std::min(m, 1.0f);
        float pressure = std::max(0.0f, m - 1.0f);

        int r = (int)(pressure * 180);
        int g = (int)(fill * 140 + 50);
        int b = (int)(fill * 255);

        sf::Color col(r, g, b);

        
        if (y > 0 && cellAt(x, y - 1).mass <= MIN_MASS)
        {
            col.r += 30;
            col.g += 30;
            col.b += 30;
        }

        
        bool edge =
            (y > 0 && cellAt(x, y - 1).mass <= MIN_MASS) ||
            (y < largeur - 1 && cellAt(x, y + 1).mass <= MIN_MASS) ||
            (x > 0 && cellAt(x - 1, y).mass <= MIN_MASS) ||
            (x < hauteur - 1 && cellAt(x + 1, y).mass <= MIN_MASS);

        if (edge)
        {
            col.r = (uint8_t)(col.r * 0.7f);
            col.g = (uint8_t)(col.g * 0.7f);
            col.b = (uint8_t)(col.b * 0.7f);
        }


        int noise = rand() % 20 - 10;
        if (settings.stableNoise)
            noise = ((x * 37 + y * 17) % 21) - 10;

        col.r = std::clamp(col.r + noise, 0, 255);
        col.g = std::clamp(col.g + noise, 0, 255);
        col.b = std::clamp(col.b + noise, 0, 255);

        return col;
    }

    int blue = std::min(255, (int)(c.mass * 255));
    return sf::Color(0, 0, blue);
}

void addCellQuad(sf::VertexArray &vertices, int x, int y, sf::Color color)
{
    float left = x * cellule_taille * 1.0f;
    float top = y * cellule_taille * 1.0f;
    float right = left + cellule_taille - 1.0f;
    float bottom = top + cellule_taille - 1.0f;

    vertices.append(sf::Vertex({ left, top }, color));
    vertices.append(sf::Vertex({ right, top }, color));
    vertices.append(sf::Vertex({ right, bottom }, color));

    vertices.append(sf::Vertex({ left, top }, color));
    vertices.append(sf::Vertex({ right, bottom }, color));
    vertices.append(sf::Vertex({ left, bottom }, color));
}

void drawGrid(sf::RenderWindow &window)
{
    ZoneScopedN("drawGrid");

    window.clear(sf::Color::Black);

    static sf::VertexArray vertices(sf::PrimitiveType::Triangles);
    vertices.clear();

    for (int y = 0; y < largeur; y++)
    {
        for (int x = 0; x < hauteur; x++)
        {
            Cell &c = cellAt(x, y);

            if (c.type == EMPTY)
                continue;

            if (c.type == WALL)
            {
                addCellQuad(vertices, x, y, sf::Color(200, 200, 200));
            }
            if (c.type == WATER)
            {
                addCellQuad(vertices, x, y, getWaterColor(x, y, c));
            }
        }
    }

    window.draw(vertices);
}
