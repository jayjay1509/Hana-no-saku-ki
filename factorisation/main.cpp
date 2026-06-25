#include "Calculateur.hpp"
#include "Rendu.hpp"

int main()
{
    Calculateur calc;
    Rendu       rendu;

    while (rendu.window.isOpen())
    {
        if (!rendu.handleEvents()) break;

        rendu.handleInput(calc);
        calc.updateFluid();
        rendu.draw(calc);
    }
}
