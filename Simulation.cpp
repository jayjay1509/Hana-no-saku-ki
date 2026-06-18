#include "Simulation.hpp"
#include "Settings.hpp"

#include <algorithm>
#include <tracy/Tracy.hpp>

std::vector<std::vector<Cell>> grid(largeur, std::vector<Cell>(hauteur));

bool inBounds(int x, int y)
{
    return y >= 0 && y < largeur && x >= 0 && x < hauteur;
}

bool isSolid(int x, int y)
{
    return !inBounds(x, y) || grid[y][x].type == WALL;
}

float getStableState(float totalMass)
{
    float maxComp = settings.useCompressibility ? settings.compressibility : MAX_COMP;

    if (totalMass <= MAX_MASS)
        return MAX_MASS;
    else if (totalMass < 2 * MAX_MASS + maxComp)
        return (MAX_MASS * MAX_MASS + totalMass * maxComp) / (MAX_MASS + maxComp);
    else
        return (totalMass + maxComp) / 2.0f;
}

float applyFlowSettings(float flow)
{
    if (settings.useMaxFlow)
        flow = std::min(flow, settings.maxFlow);

    if (settings.useViscosity)
        flow *= settings.viscosity;

    return flow;
}

void updateFluid()
{
    ZoneScopedN("updateFluid");

    auto newGrid = grid;
    static bool scanLeftToRight = true;
    int startX = 0;
    int endX = hauteur;
    int stepX = 1;

    if (settings.alternateUpdateDirection && !scanLeftToRight)
    {
        startX = hauteur - 1;
        endX = -1;
        stepX = -1;
    }

    for (int y = largeur - 1; y >= 0; y--)
    {
        for (int x = startX; x != endX; x += stepX)
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
                flow = applyFlowSettings(flow);

                if (flow > MIN_MASS)
                {
                    newGrid[y][x].mass -= flow;
                    newGrid[y + 1][x].mass += flow;
                    newGrid[y + 1][x].type = WATER;
                    mass -= flow;
                }
            }

           
            int firstDir = 1;
            int secondDir = -1;

            if (!scanLeftToRight)
            {
                firstDir = -1;
                secondDir = 1;
            }

            for (int dir : {firstDir, secondDir})
            {
                if (isSolid(x + dir, y)) continue;

                float side = grid[y][x + dir].mass;
                float flow = (mass - side) / 4.0f;
                flow = std::max(0.f, std::min(flow, mass));
                flow = applyFlowSettings(flow);

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
                flow = applyFlowSettings(flow);

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

    if (settings.alternateUpdateDirection)
        scanLeftToRight = !scanLeftToRight;
}

void resetGrid()
{
    grid = std::vector<std::vector<Cell>>(largeur, std::vector<Cell>(hauteur));
}
