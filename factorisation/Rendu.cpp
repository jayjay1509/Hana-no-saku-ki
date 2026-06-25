#include "Rendu.hpp"
#include <algorithm>
#include <cstdlib>

Rendu::Rendu()
    : window(
        sf::VideoMode({ HAUTEUR * CELLULE_TAILLE, LARGEUR * CELLULE_TAILLE }),
        "Fluid Simulation - Walls + Pressure"
      ),
      cellShape({ CELLULE_TAILLE - 1.f, CELLULE_TAILLE - 1.f })
{
    window.setFramerateLimit(60);
}

bool Rendu::handleEvents()
{
    while (auto event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
            return false;
        }
    }
    return true;
}

void Rendu::handleInput(Calculateur& calc)
{
    auto pos = sf::Mouse::getPosition(window);
    int x = pos.x / CELLULE_TAILLE;
    int y = pos.y / CELLULE_TAILLE;

    // Clic gauche → eau
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
        if (calc.inBounds(x, y) && calc.grid[y][x].type != WALL)
        {
            calc.grid[y][x].mass = MAX_MASS;
            calc.grid[y][x].type = WATER;
        }
    }

    // Clic droit → mur
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
    {
        if (calc.inBounds(x, y))
        {
            calc.grid[y][x].type = WALL;
            calc.grid[y][x].mass = 0.0f;
        }
    }

    // Clic milieu → effacer
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
    {
        if (calc.inBounds(x, y))
        {
            calc.grid[y][x].type = EMPTY;
            calc.grid[y][x].mass = 0.0f;
        }
    }

    // Touche P → toggle pression
    if (keyPressedOnce(sf::Keyboard::Key::P))
        showPressure = !showPressure;
}

bool Rendu::keyPressedOnce(sf::Keyboard::Key key)
{
    static std::map<sf::Keyboard::Key, bool> state;
    bool pressed = sf::Keyboard::isKeyPressed(key);
    bool result  = pressed && !state[key];
    state[key]   = pressed;
    return result;
}

sf::Color Rendu::computeWaterColor(const Calculateur& calc, int x, int y) const
{
    const Cell& c  = calc.grid[y][x];
    float fill     = std::min(c.mass, 1.0f);
    float pressure = std::max(0.0f, c.mass - 1.0f);

    sf::Color col(
        (int)(pressure * 180),
        (int)(fill * 140 + 50),
        (int)(fill * 255)
    );

    // Surface brillante
    if (y > 0 && calc.grid[y - 1][x].mass <= MIN_MASS)
    {
        col.r += 30;
        col.g += 30;
        col.b += 30;
    }

    // Bord sombre
    bool edge =
        (y > 0            && calc.grid[y-1][x].mass <= MIN_MASS) ||
        (y < LARGEUR - 1  && calc.grid[y+1][x].mass <= MIN_MASS) ||
        (x > 0            && calc.grid[y][x-1].mass <= MIN_MASS) ||
        (x < HAUTEUR - 1  && calc.grid[y][x+1].mass <= MIN_MASS);

    if (edge)
    {
        col.r = (sf::Uint8)(col.r * 0.7f);
        col.g = (sf::Uint8)(col.g * 0.7f);
        col.b = (sf::Uint8)(col.b * 0.7f);
    }

    // Bruit
    int noise = rand() % 20 - 10;
    col.r = (sf::Uint8)std::clamp((int)col.r + noise, 0, 255);
    col.g = (sf::Uint8)std::clamp((int)col.g + noise, 0, 255);
    col.b = (sf::Uint8)std::clamp((int)col.b + noise, 0, 255);

    return col;
}

void Rendu::draw(const Calculateur& calc)
{
    window.clear(sf::Color::Black);

    for (int y = 0; y < LARGEUR; y++)
    {
        for (int x = 0; x < HAUTEUR; x++)
        {
            const Cell& c = calc.grid[y][x];
            if (c.type == EMPTY) continue;

            if (c.type == WALL)
            {
                cellShape.setFillColor(sf::Color(200, 200, 200));
            }
            else if (c.type == WATER)
            {
                if (showPressure)
                    cellShape.setFillColor(computeWaterColor(calc, x, y));
                else
                    cellShape.setFillColor(sf::Color(0, 0, std::min(255, (int)(c.mass * 255))));
            }

            cellShape.setPosition({ x * CELLULE_TAILLE * 1.f, y * CELLULE_TAILLE * 1.f });
            window.draw(cellShape);
        }
    }

    window.display();
}
