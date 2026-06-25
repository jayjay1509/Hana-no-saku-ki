#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include "Calculateur.hpp"

class Rendu
{
public:
    sf::RenderWindow   window;
    sf::RectangleShape cellShape;
    bool               showPressure = true;

    Rendu();

    // Retourne false si la fenêtre doit se fermer
    bool handleEvents();

    // Gestion des entrées souris / clavier → modifie la grille
    void handleInput(Calculateur& calc);

    // Dessine toute la grille
    void draw(const Calculateur& calc);

private:
    bool keyPressedOnce(sf::Keyboard::Key key);
    sf::Color computeWaterColor(const Calculateur& calc, int x, int y) const;
};
