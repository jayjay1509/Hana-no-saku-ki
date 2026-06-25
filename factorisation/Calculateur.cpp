#include "Calculateur.hpp"
#include <algorithm>
#include <cmath>

Calculateur::Calculateur()
    : grid(LARGEUR, std::vector<Cell>(HAUTEUR))
{}

bool Calculateur::inBounds(int x, int y) const
{
    return y >= 0 && y < LARGEUR && x >= 0 && x < HAUTEUR;
}

bool Calculateur::isSolid(int x, int y) const
{
    return !inBounds(x, y) || grid[y][x].type == WALL;
}

float Calculateur::getStableState(float totalMass) const
{
    if (totalMass <= MAX_MASS)
        return MAX_MASS;
    if (totalMass < 2 * MAX_MASS + MAX_COMP)
        return (MAX_MASS * MAX_MASS + totalMass * MAX_COMP) / (MAX_MASS + MAX_COMP);
    return (totalMass + MAX_COMP) / 2.0f;
}

void Calculateur::updateFluid()
{
    auto newGrid = grid;

    for (int y = LARGEUR - 1; y >= 0; y--)
    {
        for (int x = 0; x < HAUTEUR; x++)
        {
            Cell& cell = grid[y][x];
            if (cell.type != WATER || cell.mass <= MIN_MASS)
                continue;

            float mass = cell.mass;

            // ↓ BAS
            if (!isSolid(x, y + 1))
            {
                float below = grid[y + 1][x].mass;
                float flow  = getStableState(mass + below) - below;
                flow = std::max(0.f, std::min(flow, mass));
                if (flow > MIN_MASS)
                {
                    newGrid[y][x].mass     -= flow;
                    newGrid[y+1][x].mass   += flow;
                    newGrid[y+1][x].type    = WATER;
                    mass -= flow;
                }
            }

            // ← → CÔTÉS
            for (int dir : {-1, 1})
            {
                if (isSolid(x + dir, y)) continue;
                float side = grid[y][x + dir].mass;
                float flow = (mass - side) / 4.0f;
                flow = std::max(0.f, std::min(flow, mass));
                if (flow > MIN_MASS)
                {
                    newGrid[y][x].mass        -= flow;
                    newGrid[y][x + dir].mass  += flow;
                    newGrid[y][x + dir].type   = WATER;
                    mass -= flow;
                }
            }

            // ↑ HAUT (pression)
            if (!isSolid(x, y - 1))
            {
                float above = grid[y - 1][x].mass;
                float flow  = mass - getStableState(mass + above);
                flow = std::max(0.f, std::min(flow, mass));
                if (flow > MIN_MASS)
                {
                    newGrid[y][x].mass     -= flow;
                    newGrid[y-1][x].mass   += flow;
                    newGrid[y-1][x].type    = WATER;
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
