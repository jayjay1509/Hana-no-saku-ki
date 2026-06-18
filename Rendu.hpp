#pragma once

#include <SFML/Graphics.hpp>

extern bool showPressure;
extern int brushSize;

void handleEvents(sf::RenderWindow &window);
void handleInput(sf::RenderWindow &window);
void drawGrid(sf::RenderWindow &window, sf::RectangleShape &cell);
