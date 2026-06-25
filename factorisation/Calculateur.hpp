#pragma once
#include <vector>

const int CELLULE_TAILLE = 6;
const int HAUTEUR        = 200;
const int LARGEUR        = 120;

const float MAX_MASS = 1.0f;
const float MAX_COMP = 0.25f;
const float MIN_MASS = 0.0001f;

enum CellType { EMPTY, WATER, WALL };

struct Cell
{
    float    mass = 0.0f;
    CellType type = EMPTY;
};

class Calculateur
{
public:
    std::vector<std::vector<Cell>> grid;

    Calculateur();

    bool  inBounds(int x, int y) const;
    bool  isSolid(int x, int y)  const;
    float getStableState(float totalMass) const;
    void  updateFluid();
};
