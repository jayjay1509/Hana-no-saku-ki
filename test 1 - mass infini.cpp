#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>

const int cellule_taille = 6;
const int hauteur = 200;
const int largeur = 120;

// ===== FLUID PARAMS =====
const float MAX_MASS = 1.0f;
const float MAX_COMP = 0.25f;
const float MIN_MASS = 0.0001f;

bool showPressure = true;

// ===== INPUT EDGE =====
bool keyPressedOnce(sf::Keyboard::Key key)
{
    static std::map<sf::Keyboard::Key, bool> state;

    bool pressed = sf::Keyboard::isKeyPressed(key);
    bool result = pressed && !state[key];

    state[key] = pressed;
    return result;
}

// ===== CELL =====
enum CellType { EMPTY, WATER, WALL };

struct Cell
{
    float mass = 0.0f;
    CellType type = EMPTY;
};

std::vector<std::vector<Cell>> grid(largeur, std::vector<Cell>(hauteur));
std::vector<std::vector<Cell>> buffer(largeur, std::vector<Cell>(hauteur));

bool inBounds(int x, int y)
{
    return y >= 0 && y < largeur && x >= 0 && x < hauteur;
}

// ===== PRESSURE =====
float getStableState(float totalMass)
{
    if (totalMass <= MAX_MASS)
        return MAX_MASS;
    else if (totalMass < 2 * MAX_MASS + MAX_COMP)
        return (MAX_MASS * MAX_MASS + totalMass * MAX_COMP) / (MAX_MASS + MAX_COMP);
    else
        return (totalMass + MAX_COMP) / 2.0f;
}

// ===== SIMULATION =====
void updateFluid()
{
    std::swap(grid, buffer);

    const auto& oldGrid = buffer;
    auto& newGrid = grid;

    // ===== RESET PROPRE =====
    for (int y = 0; y < largeur; y++)
    {
        for (int x = 0; x < hauteur; x++)
        {
            newGrid[y][x].mass = 0.0f;
            newGrid[y][x].type = EMPTY;
        }
    }

    // ===== RESTORE WALLS =====
    for (int y = 0; y < largeur; y++)
    {
        for (int x = 0; x < hauteur; x++)
        {
            if (oldGrid[y][x].type == WALL)
            {
                newGrid[y][x].type = WALL;
            }
        }
    }

    // ===== FLUID =====
    for (int y = largeur - 1; y >= 0; y--)
    {
        for (int x = 0; x < hauteur; x++)
        {
            const Cell& c = oldGrid[y][x];

            if (c.type != WATER || c.mass <= MIN_MASS)
                continue;

            float mass = c.mass;
            float remaining = mass;

            // ===== DOWN =====
            if (inBounds(x, y + 1) && oldGrid[y + 1][x].type != WALL)
            {
                float below = oldGrid[y + 1][x].mass;

                float flow = getStableState(mass + below) - below;
                flow = std::clamp(flow, 0.f, remaining);

                newGrid[y + 1][x].mass += flow;
                newGrid[y + 1][x].type = WATER;

                remaining -= flow;
            }

            // ===== SIDE =====
            for (int dir : {-1, 1})
            {
                int nx = x + dir;

                if (!inBounds(nx, y) || oldGrid[y][nx].type == WALL)
                    continue;

                float side = oldGrid[y][nx].mass;

                float flow = (remaining - side) * 0.25f;
                flow = std::clamp(flow, 0.f, remaining);

                newGrid[y][nx].mass += flow;
                newGrid[y][nx].type = WATER;

                remaining -= flow;
            }

            // ===== REST =====
            if (remaining > MIN_MASS)
            {
                newGrid[y][x].mass += remaining;
                newGrid[y][x].type = WATER;
            }
        }
    }
}

// ===== MAIN =====
int main()
{
    sf::RenderWindow window(
        sf::VideoMode({ hauteur * cellule_taille, largeur * cellule_taille }),
        "Fluid Simulation FIXED"
    );

    sf::RectangleShape cell({ cellule_taille - 1.f, cellule_taille - 1.f });

    while (window.isOpen())
    {
        // ===== EVENTS =====
        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        auto worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        int x = worldPos.x / cellule_taille;
        int y = worldPos.y / cellule_taille;

        // ===== INPUT =====
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
        {
            if (inBounds(x, y) && grid[y][x].type != WALL)
            {
                grid[y][x].mass = MAX_MASS;
                grid[y][x].type = WATER;
            }
        }

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        {
            if (inBounds(x, y))
            {
                grid[y][x].type = WALL;
                grid[y][x].mass = 0;
            }
        }

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
        {
            if (inBounds(x, y))
            {
                grid[y][x].type = EMPTY;
                grid[y][x].mass = 0;
            }
        }

        if (keyPressedOnce(sf::Keyboard::Key::P))
            showPressure = !showPressure;

        // ===== SIM =====
        updateFluid();
        

        // ===== RENDER =====
        window.clear(sf::Color::Black);

        for (int y = 0; y < largeur; y++)
        {
            for (int x = 0; x < hauteur; x++)
            {
                auto& c = grid[y][x];

                if (c.type == EMPTY)
                    continue;

                if (c.type == WALL)
                {
                    cell.setFillColor(sf::Color(200, 200, 200));
                }
                else
                {
                    if (c.mass <= MIN_MASS)
                        continue;

                    float m = c.mass;

                    float fill = std::min(m, 1.0f);
                    float pressure = std::max(0.0f, m - 1.0f);

                    float r = pressure * 90.f;
                    float g = 40.f + fill * 60.f;
                    float b = 120.f + fill * 120.f;

                    sf::Color col(
                        (uint8_t)std::clamp(r, 0.f, 255.f),
                        (uint8_t)std::clamp(g, 0.f, 255.f),
                        (uint8_t)std::clamp(b, 0.f, 255.f)
                    );

                    cell.setFillColor(col);
                }

                cell.setPosition({ x * cellule_taille * 1.f, y * cellule_taille * 1.f });
                window.draw(cell);
            }
        }

        window.display();
    }
}