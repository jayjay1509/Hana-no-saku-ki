#include "Simulation.hpp"
#include "Settings.hpp"

#include <algorithm>
#include <chrono>
#include <tracy/Tracy.hpp>

int hauteur = 200;
int largeur = 120;
std::vector<Cell> grid(largeur * hauteur);

namespace
{
    std::vector<Cell> nextGrid(largeur * hauteur);
    bool pressureDemoSourceActive = false;
    int pressureSourceX0 = 0;
    int pressureSourceX1 = 0;
    int pressureSourceY0 = 0;
    int pressureSourceY1 = 0;
    std::chrono::steady_clock::time_point pressureSourceStartTime;
    float pressureSourceDurationSeconds = 0.0f;

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

    void setCell(int x, int y, CellType type, float mass)
    {
        if (!inBounds(x, y))
            return;

        cellAt(x, y).type = type;
        cellAt(x, y).mass = mass;
    }

    void fillPressureDemoSource()
    {
        if (!pressureDemoSourceActive)
            return;

        ZoneScopedN("fillPressureDemoSource");

        float elapsedSeconds = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - pressureSourceStartTime).count();

        if (elapsedSeconds >= pressureSourceDurationSeconds)
        {
            pressureDemoSourceActive = false;
            return;
        }

        for (int y = pressureSourceY0; y <= pressureSourceY1; y++)
        {
            for (int x = pressureSourceX0; x <= pressureSourceX1; x++)
            {
                setCell(x, y, WATER, MAX_MASS);
            }
        }
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
    fillPressureDemoSource();

    if (settings.alternateUpdateDirection)
        scanLeftToRight = !scanLeftToRight;
}

void resetGrid()
{
    pressureDemoSourceActive = false;
    grid = std::vector<Cell>(largeur * hauteur);
    nextGrid = std::vector<Cell>(largeur * hauteur);
}

void resizeGrid(int columns, int rows)
{
    hauteur = std::clamp(columns, 20, 600);
    largeur = std::clamp(rows, 20, 400);
    resetGrid();
}

void loadBenchmarkScene()
{
    resetGrid();

    int marginX = std::max(2, hauteur / 10);
    int floorY = std::max(0, largeur - std::max(2, largeur / 10));
    int wallTop = std::max(1, largeur * 3 / 8);
    int leftWall = marginX;
    int rightWall = hauteur - marginX - 1;
    int platformY = std::clamp(largeur * 2 / 3, 1, largeur - 2);
    int platformStart = hauteur * 28 / 100;
    int platformEnd = hauteur * 72 / 100;

    for (int x = marginX; x < hauteur - marginX; x++)
    {
        setCell(x, floorY, WALL, 0.0f);
    }

    for (int y = wallTop; y <= floorY; y++)
    {
        setCell(leftWall, y, WALL, 0.0f);
        setCell(rightWall, y, WALL, 0.0f);
    }

    for (int x = platformStart; x < platformEnd; x++)
    {
        setCell(x, platformY, WALL, 0.0f);
    }

    for (int y = largeur * 20 / 100; y < largeur * 58 / 100; y++)
    {
        for (int x = hauteur * 35 / 100; x < hauteur * 65 / 100; x++)
        {
            setCell(x, y, WATER, MAX_MASS);
        }
    }

    for (int y = largeur * 12 / 100; y < largeur * 25 / 100; y++)
    {
        for (int x = hauteur * 15 / 100; x < hauteur * 28 / 100; x++)
        {
            setCell(x, y, WATER, MAX_MASS);
        }
    }
}

void loadPressureDemoScene()
{
    resetGrid();

    int topY = std::clamp(largeur * 10 / 100, 1, largeur - 4);
    int floorY = std::clamp(largeur * 86 / 100, topY + 6, largeur - 2);
    int channelTop = std::clamp(largeur * 70 / 100, topY + 3, floorY - 2);

    int leftWall = std::clamp(hauteur * 15 / 100, 1, hauteur - 8);
    int middleWall = std::clamp(hauteur * 50 / 100, leftWall + 4, hauteur - 5);
    int rightWall = std::clamp(hauteur * 85 / 100, middleWall + 4, hauteur - 2);

    for (int x = leftWall; x <= rightWall; x++)
        setCell(x, floorY, WALL, 0.0f);

    for (int y = topY; y <= floorY; y++)
    {
        setCell(leftWall, y, WALL, 0.0f);
        setCell(rightWall, y, WALL, 0.0f);
    }

    for (int y = topY; y <= channelTop; y++)
    {
        setCell(middleWall, y, WALL, 0.0f);
    }

    int initialWaterTop = std::clamp(topY + (floorY - topY) / 2, topY + 1, floorY - 1);

    for (int y = initialWaterTop; y < floorY; y++)
    {
        for (int x = leftWall + 1; x < middleWall; x++)
        {
            setCell(x, y, WATER, MAX_MASS);
        }
    }

    int sourceWidth = std::max(3, (middleWall - leftWall) / 5);
    int sourceHeight = std::max(2, (floorY - topY) / 18);
    pressureSourceX0 = leftWall + 2;
    pressureSourceX1 = std::min(middleWall - 2, pressureSourceX0 + sourceWidth);
    pressureSourceY0 = topY + 2;
    pressureSourceY1 = std::min(floorY - 1, pressureSourceY0 + sourceHeight);
    pressureSourceDurationSeconds = std::clamp((hauteur * largeur) / 4500.0f, 3.0f, 12.0f);
    pressureSourceStartTime = std::chrono::steady_clock::now();
    pressureDemoSourceActive = true;
    fillPressureDemoSource();
}
