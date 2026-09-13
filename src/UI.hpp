#pragma once
#include "Game.hpp"
#include <SFML/Graphics.hpp>

struct Button {
  sf::RectangleShape shape;
  sf::Text text;
  sf::Color normal = sf::Color(74, 144, 226);
  sf::Color hover = sf::Color(53, 122, 189);
  sf::Color pressed = sf::Color(40, 100, 160);

  Button() = delete;
  Button(sf::Font& font, const std::string& str, sf::Vector2f pos, sf::Vector2f size, unsigned int charSize = 18);

  bool contains(sf::Vector2i p) const;
  bool contains(sf::Vector2f p) const;
  void setHovered(bool h);
  void setPosition(sf::Vector2f pos);
  void setSize(sf::Vector2f size);
  void draw(sf::RenderWindow& w) const;
};

void drawOverlay(sf::RenderWindow& w, sf::Color c = sf::Color(0, 0, 0, 150));
void centerText(sf::Text& t, float centerX, float y);

namespace UI {
  void drawMenu(sf::RenderWindow& w, sf::Font& font, Button& startButton);
  void drawHUD(sf::RenderWindow& w, sf::Font& font, Game& game, Button& restartButton);
  void drawGameOver(sf::RenderWindow& w, sf::Font& font, Game& game, Button& restartButton, Button& menuButton);
  void drawGameWon(sf::RenderWindow& w, sf::Font& font, Game& game, Button& againButton, Button& menuButton);
  void drawRestarting(sf::RenderWindow& w, sf::Font& font);
}
