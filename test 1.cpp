#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>

const int cellule_taille = 6;
const int hauteur = 200;
const int largeur = 120;



// === FLUID PARAMS ===
const float MAX_MASS = 1.0f;
const float MAX_COMP = 0.25f;
const float MIN_MASS = 0.0001f;


bool showPressure = true;
bool keyPressedOnce(sf::Keyboard::Key key)
{
    static std::map<sf::Keyboard::Key, bool> state;

    bool pressed = sf::Keyboard::isKeyPressed(key);
    bool result = pressed && !state[key];

    state[key] = pressed;
    return result;
}



// Type de cellule
enum CellType
{
    EMPTY,
    WATER,
    WALL
};

struct Cell
{
    float mass = 0.0f;
    CellType type = EMPTY;
};

std::vector<std::vector<Cell>> grid(largeur, std::vector<Cell>(hauteur));

bool inBounds(int x, int y)
{
    return y >= 0 && y < largeur && x >= 0 && x < hauteur;
}

bool isSolid(int x, int y)
{
    return !inBounds(x, y) || grid[y][x].type == WALL;
}

// Pression
float getStableState(float totalMass)
{
    if (totalMass <= MAX_MASS)
        return MAX_MASS;
    else if (totalMass < 2 * MAX_MASS + MAX_COMP)
        return (MAX_MASS * MAX_MASS + totalMass * MAX_COMP) / (MAX_MASS + MAX_COMP);
    else
        return (totalMass + MAX_COMP) / 2.0f;
}

void updateFluid()
{
    auto newGrid = grid;

    for (int y = largeur - 1; y >= 0; y--)
    {
        for (int x = 0; x < hauteur; x++)
        {
            Cell &cell = grid[y][x];

            if (cell.type != WATER || cell.mass <= MIN_MASS)
                continue;

            float mass = cell.mass;

            
            if (!isSolid(x, y + 1))
            {
                float below = grid[y + 1][x].mass;
                float flow = getStableState(mass + below) - below;
                flow = std::max(0.f, std::min(flow, mass));

                if (flow > MIN_MASS)
                {
                    newGrid[y][x].mass -= flow;
                    newGrid[y + 1][x].mass += flow;
                    newGrid[y + 1][x].type = WATER;
                    mass -= flow;
                }
            }

           
            for (int dir : {-1, 1})
            {
                if (isSolid(x + dir, y)) continue;

                float side = grid[y][x + dir].mass;
                float flow = (mass - side) / 4.0f;
                flow = std::max(0.f, std::min(flow, mass));

                if (flow > MIN_MASS)
                {
                    newGrid[y][x].mass -= flow;
                    newGrid[y][x + dir].mass += flow;
                    newGrid[y][x + dir].type = WATER;
                    mass -= flow;
                }
            }

            // ↑ UP (pression)
            if (!isSolid(x, y - 1))
            {
                float above = grid[y - 1][x].mass;
                float flow = mass - getStableState(mass + above);
                flow = std::max(0.f, std::min(flow, mass));

                if (flow > MIN_MASS)
                {
                    newGrid[y][x].mass -= flow;
                    newGrid[y - 1][x].mass += flow;
                    newGrid[y - 1][x].type = WATER;
                    mass -= flow;
                }
            }

            // Nettoyage
            if (newGrid[y][x].mass <= MIN_MASS)
            {
                newGrid[y][x].mass = 0.0f;
                if (newGrid[y][x].type != WALL)
                    newGrid[y][x].type = EMPTY;
            }
            else
            {
                newGrid[y][x].type = WATER;
            }
        }
    }

    grid = newGrid;
}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({ hauteur * cellule_taille, largeur * cellule_taille }),
        "Fluid Simulation - Walls + Pressure"
    );

    window.setFramerateLimit(60);

    sf::RectangleShape cell({ cellule_taille - 1.f, cellule_taille - 1.f });

    while (window.isOpen())
    {
        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        auto pos = sf::Mouse::getPosition(window);
        int x = pos.x / cellule_taille;
        int y = pos.y / cellule_taille;

        
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
        {
            if (inBounds(x, y) && grid[y][x].type != WALL)
            {
                grid[y][x].mass = MAX_MASS;
                grid[y][x].type = WATER;
            }
        }
        
        if (keyPressedOnce(sf::Keyboard::Key::P))
            showPressure = !showPressure;
        
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        {
            if (inBounds(x, y))
            {
                grid[y][x].type = WALL;
                grid[y][x].mass = 0.0f;
            }
        }

        
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
        {
            if (inBounds(x, y))
            {
                grid[y][x].type = EMPTY;
                grid[y][x].mass = 0.0f;
            }
        }

        updateFluid();

        window.clear(sf::Color::Black);

        for (int y = 0; y < largeur; y++)
        {
            for (int x = 0; x < hauteur; x++)
            {
                Cell &c = grid[y][x];

                if (c.type == EMPTY)
                    continue;

                if (c.type == WALL)
                {
                    cell.setFillColor(sf::Color(200, 200, 200));
                }
                if (c.type == WATER)
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

                        // surface
                        if (y > 0 && grid[y - 1][x].mass <= MIN_MASS)
                        {
                            col.r += 30;
                            col.g += 30;
                            col.b += 30;
                        }

                        // edges
                        bool edge =
                            (y > 0 && grid[y - 1][x].mass <= MIN_MASS) ||
                            (y < largeur - 1 && grid[y + 1][x].mass <= MIN_MASS) ||
                            (x > 0 && grid[y][x - 1].mass <= MIN_MASS) ||
                            (x < hauteur - 1 && grid[y][x + 1].mass <= MIN_MASS);

                        if (edge)
                        {
                            col.r *= 0.7f;
                            col.g *= 0.7f;
                            col.b *= 0.7f;
                        }

                        // bruit
                        int noise = rand() % 20 - 10;
                        col.r = std::clamp(col.r + noise, 0, 255);
                        col.g = std::clamp(col.g + noise, 0, 255);
                        col.b = std::clamp(col.b + noise, 0, 255);

                        cell.setFillColor(col);
                    }
                    else
                    {
                        int blue = std::min(255, (int)(c.mass * 255));
                        cell.setFillColor(sf::Color(0, 0, blue));
                    }
                }

                cell.setPosition({ x * cellule_taille * 1.f, y * cellule_taille * 1.f });
                window.draw(cell);
            }
        }

        window.display();
    }
}