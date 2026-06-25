#include "Simulation.hpp"
#include "Settings.hpp"

#include <algorithm>
#include <tracy/Tracy.hpp>

std::vector<Cell> grid(largeur * hauteur);

namespace
{
    std::vector<Cell> nextGrid(largeur * hauteur);

    Cell &nextCellAt(int x, int y)
    {
        return nextGrid[gridIndex(x, y)];
    }

    void clearNextGrid()
    {
        ZoneScopedN("clearNextGrid");

        std::fill(nextGrid.begin(), nextGrid.end(), Cell{});
    }

    void restoreWalls()
    {
        ZoneScopedN("restoreWalls");

        for (int y = 0; y < largeur; y++)
        {
            for (int x = 0; x < hauteur; x++)
            {
                if (cellAt(x, y).type == WALL)
                    nextCellAt(x, y).type = WALL;
            }
        }
    }

    void addWater(int x, int y, float mass)
    {
        Cell &target = nextCellAt(x, y);

        if (mass <= MIN_MASS || target.type == WALL)
            return;

        target.mass += mass;
        target.type = WATER;
    }
}

bool inBounds(int x, int y)
{
    return y >= 0 && y < largeur && x >= 0 && x < hauteur;
}

int gridIndex(int x, int y)
{
    return y * hauteur + x;
}

Cell &cellAt(int x, int y)
{
    return grid[gridIndex(x, y)];
}

bool isSolid(int x, int y)
{
    return !inBounds(x, y) || cellAt(x, y).type == WALL;
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

GridStats computeGridStats()
{
    ZoneScopedN("computeGridStats");

    GridStats stats;

    for (int y = 0; y < largeur; y++)
    {
        for (int x = 0; x < hauteur; x++)
        {
            const Cell &cell = cellAt(x, y);

            if (cell.type == WATER && cell.mass > MIN_MASS)
            {
                stats.waterCells++;
                stats.totalMass += cell.mass;
            }
            else if (cell.type == WALL)
            {
                stats.wallCells++;
            }
        }
    }

    return stats;
}

void updateFluid()
{
    ZoneScopedN("updateFluid");

    clearNextGrid();
    restoreWalls();

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
            Cell &cell = cellAt(x, y);

            if (cell.type != WATER || cell.mass <= MIN_MASS)
                continue;

            float remainingMass = cell.mass;

            
            if (!isSolid(x, y + 1))
            {
                float below = cellAt(x, y + 1).mass;
                float flow = getStableState(cell.mass + below) - below;
                flow = std::max(0.f, std::min(flow, remainingMass));
                flow = applyFlowSettings(flow);

                if (flow > MIN_MASS)
                {
                    addWater(x, y + 1, flow);
                    remainingMass -= flow;
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

                float side = cellAt(x + dir, y).mass;
                float flow = (remainingMass - side) / 4.0f;
                flow = std::max(0.f, std::min(flow, remainingMass));
                flow = applyFlowSettings(flow);

                if (flow > MIN_MASS)
                {
                    addWater(x + dir, y, flow);
                    remainingMass -= flow;
                }
            }

            // ↑ UP (pression)
            if (!isSolid(x, y - 1))
            {
                float above = cellAt(x, y - 1).mass;
                float flow = remainingMass - getStableState(remainingMass + above);
                flow = std::max(0.f, std::min(flow, remainingMass));
                flow = applyFlowSettings(flow);

                if (flow > MIN_MASS)
                {
                    addWater(x, y - 1, flow);
                    remainingMass -= flow;
                }
            }

            addWater(x, y, remainingMass);
        }
    }

    std::swap(grid, nextGrid);

    if (settings.alternateUpdateDirection)
        scanLeftToRight = !scanLeftToRight;
}

void resetGrid()
{
    grid = std::vector<Cell>(largeur * hauteur);
    nextGrid = std::vector<Cell>(largeur * hauteur);
}

void loadBenchmarkScene()
{
    resetGrid();

    for (int x = 20; x < hauteur - 20; x++)
    {
        cellAt(x, largeur - 12).type = WALL;
        cellAt(x, largeur - 12).mass = 0.0f;
    }

    for (int y = 45; y < largeur - 12; y++)
    {
        cellAt(20, y).type = WALL;
        cellAt(20, y).mass = 0.0f;

        cellAt(hauteur - 21, y).type = WALL;
        cellAt(hauteur - 21, y).mass = 0.0f;
    }

    for (int x = 55; x < 145; x++)
    {
        cellAt(x, 78).type = WALL;
        cellAt(x, 78).mass = 0.0f;
    }

    for (int y = 25; y < 70; y++)
    {
        for (int x = 70; x < 130; x++)
        {
            cellAt(x, y).type = WATER;
            cellAt(x, y).mass = MAX_MASS;
        }
    }

    for (int y = 15; y < 30; y++)
    {
        for (int x = 30; x < 55; x++)
        {
            cellAt(x, y).type = WATER;
            cellAt(x, y).mass = MAX_MASS;
        }
    }
}
