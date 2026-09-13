#include "UI.hpp"
#include <cmath>

Button::Button(sf::Font& font, const std::string& str, sf::Vector2f pos, sf::Vector2f size, unsigned int charSize)
    : text(font, str, charSize) {
  shape.setPosition(pos);
  shape.setSize(size);
  shape.setFillColor(normal);
  shape.setOutlineThickness(0.f);
  // Center text inside button
  text.setFillColor(sf::Color::White);
  sf::FloatRect bounds = text.getLocalBounds();
  // SFML 3: getLocalBounds returns rect with position offset
  float textX = pos.x + (size.x - bounds.size.x) / 2.f - bounds.position.x;
  float textY = pos.y + (size.y - bounds.size.y) / 2.f - bounds.position.y - 2.f;
  text.setPosition(sf::Vector2f(textX, textY));
}

bool Button::contains(sf::Vector2i p) const {
  return contains(sf::Vector2f(static_cast<float>(p.x), static_cast<float>(p.y)));
}

bool Button::contains(sf::Vector2f p) const {
  return shape.getGlobalBounds().contains(p);
}

void Button::setHovered(bool h) {
  shape.setFillColor(h ? hover : normal);
}

void Button::setPosition(sf::Vector2f pos) {
  sf::Vector2f delta = pos - shape.getPosition();
  shape.setPosition(pos);
  text.setPosition(text.getPosition() + delta);
}

void Button::setSize(sf::Vector2f size) {
  shape.setSize(size);
}

void Button::draw(sf::RenderWindow& w) const {
  w.draw(shape);
  w.draw(text);
}

void drawOverlay(sf::RenderWindow& w, sf::Color c) {
  sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(w.getSize().x), static_cast<float>(w.getSize().y)));
  overlay.setFillColor(c);
  overlay.setPosition(sf::Vector2f(0.f, 0.f));
  w.draw(overlay);
}

void centerText(sf::Text& t, float centerX, float y) {
  sf::FloatRect b = t.getLocalBounds();
  float x = centerX - (b.size.x / 2.f) - b.position.x;
  float ty = y - b.position.y;
  t.setPosition(sf::Vector2f(x, ty));
}

namespace UI {

void drawMenu(sf::RenderWindow& w, sf::Font& font, Button& startButton) {
  // Title
  sf::Text title(font, "MINESWEEPER", 42);
  title.setFillColor(sf::Color(30, 30, 30));
  title.setStyle(sf::Text::Bold);
  centerText(title, 320.f, 90.f);
  w.draw(title);

  sf::Text subtitle(font, "Classic puzzle  •  12 x 12  •  ~20% mines", 14);
  subtitle.setFillColor(sf::Color(60, 60, 60));
  centerText(subtitle, 320.f, 135.f);
  w.draw(subtitle);

  startButton.draw(w);

  sf::Text hint(font, "Left click: reveal  |  Right click: flag  |  R: restart  |  Esc: menu", 12);
  hint.setFillColor(sf::Color(50, 50, 50));
  centerText(hint, 320.f, 315.f);
  w.draw(hint);

  sf::Text hint2(font, "First click is always safe", 12);
  hint2.setFillColor(sf::Color(80, 80, 80));
  hint2.setStyle(sf::Text::Italic);
  centerText(hint2, 320.f, 335.f);
  w.draw(hint2);
}

void drawHUD(sf::RenderWindow& w, sf::Font& font, Game& game, Button& restartButton) {
  sf::RectangleShape header(sf::Vector2f(640.f, static_cast<float>(HEADER_H)));
  header.setPosition(sf::Vector2f(0.f, 0.f));
  header.setFillColor(sf::Color(45, 45, 48));
  w.draw(header);

  // Mines counter: total mines - flags (or flags left). Show flags / mines
  int flags = game.flagCount();
  int mines = 0;
  if (game.state == PLAYING || game.state == GAME_OVER || game.state == GAME_WON) {
    mines = game.mineCount();
  } else {
    mines = 0;
  }
  // If no mines placed yet (MENU), estimate
  std::string mineStr;
  if (game.state == MENU) {
    mineStr = "Mines: --";
  } else {
    int remaining = mines - flags;
    if (remaining < 0) remaining = 0;
    mineStr = "Mines: " + std::to_string(remaining);
  }

  sf::Text mineText(font, mineStr, 16);
  mineText.setFillColor(sf::Color::White);
  mineText.setPosition(sf::Vector2f(16.f, 14.f));
  w.draw(mineText);

  // Flags text
  sf::Text flagText(font, "Flags: " + std::to_string(flags), 14);
  flagText.setFillColor(sf::Color(200, 200, 200));
  flagText.setPosition(sf::Vector2f(16.f, 30.f));
  // only show when playing
  if (game.state != MENU)
    w.draw(flagText);

  // Restart button centered in header
  restartButton.draw(w);

  // Timer
  int seconds = 0;
  if (game.state == PLAYING) {
    seconds = static_cast<int>(game.gameClock.getElapsedTime().asSeconds());
  } else if (game.state == GAME_OVER || game.state == GAME_WON) {
    seconds = static_cast<int>(game.elapsedSeconds);
  }
  // Format mm:ss
  int mm = seconds / 60;
  int ss = seconds % 60;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
  sf::Text timeText(font, std::string("Time: ") + buf, 16);
  timeText.setFillColor(sf::Color::White);
  // right align
  sf::FloatRect tb = timeText.getLocalBounds();
  float tx = 640.f - tb.size.x - tb.position.x - 16.f;
  timeText.setPosition(sf::Vector2f(tx, 14.f));
  w.draw(timeText);
}

void drawGameOver(sf::RenderWindow& w, sf::Font& font, Game& game, Button& restartButton, Button& menuButton) {
  (void)game;
  drawOverlay(w, sf::Color(0, 0, 0, 160));

  sf::Text title(font, "GAME OVER", 38);
  title.setFillColor(sf::Color(231, 76, 60));
  title.setStyle(sf::Text::Bold);
  centerText(title, 320.f, 150.f);
  w.draw(title);

  sf::Text sub(font, "You hit a mine!", 16);
  sub.setFillColor(sf::Color::White);
  centerText(sub, 320.f, 200.f);
  w.draw(sub);

  restartButton.draw(w);
  menuButton.draw(w);

  sf::Text hint(font, "Press R to restart  •  Esc for menu", 12);
  hint.setFillColor(sf::Color(200, 200, 200));
  centerText(hint, 320.f, 350.f);
  w.draw(hint);
}

void drawGameWon(sf::RenderWindow& w, sf::Font& font, Game& game, Button& againButton, Button& menuButton) {
  drawOverlay(w, sf::Color(0, 0, 0, 160));

  sf::Text title(font, "YOU WON!", 38);
  title.setFillColor(sf::Color(46, 204, 113));
  title.setStyle(sf::Text::Bold);
  centerText(title, 320.f, 150.f);
  w.draw(title);

  int secs = static_cast<int>(game.elapsedSeconds);
  int mm = secs / 60;
  int ss = secs % 60;
  char buf[32];
  std::snprintf(buf, sizeof(buf), "Time: %02d:%02d", mm, ss);
  sf::Text time(font, buf, 16);
  time.setFillColor(sf::Color::White);
  centerText(time, 320.f, 200.f);
  w.draw(time);

  againButton.draw(w);
  menuButton.draw(w);

  sf::Text hint(font, "Press R to play again  •  Esc for menu", 12);
  hint.setFillColor(sf::Color(200, 200, 200));
  centerText(hint, 320.f, 350.f);
  w.draw(hint);
}

void drawRestarting(sf::RenderWindow& w, sf::Font& font) {
  drawOverlay(w, sf::Color(0, 0, 0, 180));
  sf::Text t(font, "Restarting...", 24);
  t.setFillColor(sf::Color::White);
  centerText(t, 320.f, 220.f);
  w.draw(t);
}

} // namespace UI
