#pragma once

#include <vector>

const int cellule_taille = 6;
const int hauteur = 200;
const int largeur = 120;

const float MAX_MASS = 1.0f;
const float MAX_COMP = 0.25f;
const float MIN_MASS = 0.0001f;

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

extern std::vector<std::vector<Cell>> grid;

bool inBounds(int x, int y);
bool isSolid(int x, int y);
float getStableState(float totalMass);
void updateFluid();
void resetGrid();
