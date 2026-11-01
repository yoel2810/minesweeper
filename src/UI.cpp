#include "UI.hpp"

Button::Button(sf::Font &font, const std::string &str, sf::Vector2f pos,
               sf::Vector2f size, unsigned int charSize)
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
  float textY =
      pos.y + (size.y - bounds.size.y) / 2.f - bounds.position.y - 2.f;
  text.setPosition(sf::Vector2f(textX, textY));
}

bool Button::contains(sf::Vector2i p) const {
  return contains(
      sf::Vector2f(static_cast<float>(p.x), static_cast<float>(p.y)));
}

bool Button::contains(sf::Vector2f p) const {
  return shape.getGlobalBounds().contains(p);
}

void Button::setHovered(bool h) { shape.setFillColor(h ? hover : normal); }

void Button::setPosition(sf::Vector2f pos) {
  sf::Vector2f delta = pos - shape.getPosition();
  shape.setPosition(pos);
  text.setPosition(text.getPosition() + delta);
}

void Button::setSize(sf::Vector2f size) { shape.setSize(size); }

void Button::draw(sf::RenderWindow &w) const {
  w.draw(shape);
  w.draw(text);
}

void drawOverlay(sf::RenderWindow &w, sf::Color c) {
  sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(w.getSize().x),
                                          static_cast<float>(w.getSize().y)));
  overlay.setFillColor(c);
  overlay.setPosition(sf::Vector2f(0.f, 0.f));
  w.draw(overlay);
}

void centerText(sf::Text &t, float centerX, float y) {
  sf::FloatRect b = t.getLocalBounds();
  float x = centerX - (b.size.x / 2.f) - b.position.x;
  float ty = y - b.position.y;
  t.setPosition(sf::Vector2f(x, ty));
}

NumberField::NumberField(sf::Font &font, const std::string &name,
                         sf::Vector2f position)
    : label(font, name, 16), text(font, "12", 22) {
  shape.setPosition(position);
  shape.setSize({120.f, 38.f});
  shape.setFillColor(sf::Color::White);
  shape.setOutlineThickness(2.f);
  label.setPosition({position.x, position.y - 25.f});
  label.setFillColor(sf::Color(40, 40, 40));
  text.setPosition({position.x + 12.f, position.y + 4.f});
  text.setFillColor(sf::Color(30, 30, 30));
}

bool NumberField::contains(sf::Vector2i position) const {
  return shape.getGlobalBounds().contains(
      {static_cast<float>(position.x), static_cast<float>(position.y)});
}

void NumberField::focus(bool selected) {
  if (selected && !focused)
    replaceOnType = true;
  focused = selected;
}

void NumberField::enter(char32_t character) {
  if (!focused)
    return;
  if (character == '\b') {
    if (replaceOnType)
      value.clear();
    else if (!value.empty())
      value.pop_back();
    replaceOnType = false;
  } else if (character >= '0' && character <= '9') {
    if (replaceOnType)
      value.clear();
    if (value.size() < 2)
      value += static_cast<char>(character);
    replaceOnType = false;
  }
}

void NumberField::setPosition(sf::Vector2f position) {
  shape.setPosition(position);
  label.setPosition({position.x, position.y - 25.f});
  text.setPosition({position.x + 12.f, position.y + 4.f});
}

void NumberField::draw(sf::RenderWindow &window) {
  shape.setOutlineColor(focused ? sf::Color(35, 110, 210) : sf::Color(110, 110, 110));
  shape.setFillColor(focused && replaceOnType ? sf::Color(215, 232, 255)
                                             : sf::Color::White);
  text.setString(value + (focused && !replaceOnType ? "|" : ""));
  window.draw(shape);
  window.draw(label);
  window.draw(text);
}

namespace UI {

void layoutControls(sf::Vector2u windowSize, Button &startButton,
                    NumberField &columns, NumberField &rows,
                    Button &hudRestartButton, Button &gameOverRestartButton,
                    Button &gameOverMenuButton, Button &gameWonAgainButton,
                    Button &gameWonMenuButton) {
  const float centerX = static_cast<float>(windowSize.x) / 2.f;
  const float centerY =
      HEADER_H + (static_cast<float>(windowSize.y) - HEADER_H) / 2.f;
  startButton.setPosition({centerX - 100.f, centerY + 20.f});
  columns.setPosition({centerX - 140.f, centerY - 70.f});
  rows.setPosition({centerX + 20.f, centerY - 70.f});
  hudRestartButton.setPosition({centerX - 16.f, 8.f});
  gameOverRestartButton.setPosition({centerX - 100.f, centerY + 10.f});
  gameOverMenuButton.setPosition({centerX - 100.f, centerY + 62.f});
  gameWonAgainButton.setPosition({centerX - 100.f, centerY + 10.f});
  gameWonMenuButton.setPosition({centerX - 100.f, centerY + 62.f});
}

void drawMenu(sf::RenderWindow &w, sf::Font &font, Button &startButton,
              NumberField &columns, NumberField &rows, const std::string &error) {
  const auto size = w.getSize();
  const float centerX = static_cast<float>(size.x) / 2.f;
  const float centerY =
      HEADER_H + (static_cast<float>(size.y) - HEADER_H) / 2.f;
  // Title
  sf::Text title(font, "MINESWEEPER", 42);
  title.setFillColor(sf::Color(30, 30, 30));
  title.setStyle(sf::Text::Bold);
  centerText(title, centerX, centerY - 160.f);
  w.draw(title);

  sf::Text subtitle(font, "Custom board  |  ~20% mines", 14);
  subtitle.setFillColor(sf::Color(60, 60, 60));
  centerText(subtitle, centerX, centerY - 115.f);
  w.draw(subtitle);

  startButton.draw(w);
  columns.draw(w);
  rows.draw(w);

  sf::Text validation(font, error.empty() ? "4-30 each | Tab: switch | Enter: start" : error, 13);
  validation.setFillColor(error.empty() ? sf::Color(60, 60, 60) : sf::Color(170, 25, 25));
  centerText(validation, centerX, centerY - 20.f);
  w.draw(validation);

  sf::Text hint(
      font,
      "Left: reveal | Right: flag | R: restart | Esc: menu | F11: fullscreen",
      12);
  hint.setFillColor(sf::Color(50, 50, 50));
  centerText(hint, centerX, centerY + 105.f);
  w.draw(hint);

  sf::Text hint2(font, "First click is always safe", 12);
  hint2.setFillColor(sf::Color(80, 80, 80));
  hint2.setStyle(sf::Text::Italic);
  centerText(hint2, centerX, centerY + 125.f);
  w.draw(hint2);
}

void drawHUD(sf::RenderWindow &w, sf::Font &font, Game &game,
             Button &restartButton) {
  const float width = static_cast<float>(w.getSize().x);
  sf::RectangleShape header(sf::Vector2f(width, static_cast<float>(HEADER_H)));
  header.setPosition(sf::Vector2f(0.f, 0.f));
  header.setFillColor(sf::Color(45, 45, 48));
  w.draw(header);

  // Mines counter: total mines - flags (or flags left). Show flags / mines
  int flags = game.flagCount();
  int mines = 0;
  if (game.state == PLAYING || game.state == GAME_OVER ||
      game.state == GAME_WON) {
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
    if (remaining < 0)
      remaining = 0;
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
  float tx = width - tb.size.x - tb.position.x - 16.f;
  timeText.setPosition(sf::Vector2f(tx, 14.f));
  w.draw(timeText);
}

void drawGameOver(sf::RenderWindow &w, sf::Font &font, Game &game,
                  Button &restartButton, Button &menuButton) {
  (void)game;
  drawOverlay(w, sf::Color(0, 0, 0, 160));
  const auto size = w.getSize();
  const float centerX = static_cast<float>(size.x) / 2.f;
  const float centerY =
      HEADER_H + (static_cast<float>(size.y) - HEADER_H) / 2.f;

  sf::Text title(font, "GAME OVER", 38);
  title.setFillColor(sf::Color(231, 76, 60));
  title.setStyle(sf::Text::Bold);
  centerText(title, centerX, centerY - 90.f);
  w.draw(title);

  sf::Text sub(font, "You hit a mine!", 16);
  sub.setFillColor(sf::Color::White);
  centerText(sub, centerX, centerY - 40.f);
  w.draw(sub);

  restartButton.draw(w);
  menuButton.draw(w);

  sf::Text hint(font, "Press R to restart  •  Esc for menu", 12);
  hint.setFillColor(sf::Color(200, 200, 200));
  centerText(hint, centerX, centerY + 120.f);
  w.draw(hint);
}

void drawGameWon(sf::RenderWindow &w, sf::Font &font, Game &game,
                 Button &againButton, Button &menuButton) {
  drawOverlay(w, sf::Color(0, 0, 0, 160));
  const auto size = w.getSize();
  const float centerX = static_cast<float>(size.x) / 2.f;
  const float centerY =
      HEADER_H + (static_cast<float>(size.y) - HEADER_H) / 2.f;

  sf::Text title(font, "YOU WON!", 38);
  title.setFillColor(sf::Color(46, 204, 113));
  title.setStyle(sf::Text::Bold);
  centerText(title, centerX, centerY - 90.f);
  w.draw(title);

  int secs = static_cast<int>(game.elapsedSeconds);
  int mm = secs / 60;
  int ss = secs % 60;
  char buf[32];
  std::snprintf(buf, sizeof(buf), "Time: %02d:%02d", mm, ss);
  sf::Text time(font, buf, 16);
  time.setFillColor(sf::Color::White);
  centerText(time, centerX, centerY - 40.f);
  w.draw(time);

  againButton.draw(w);
  menuButton.draw(w);

  sf::Text hint(font, "Press R to play again  •  Esc for menu", 12);
  hint.setFillColor(sf::Color(200, 200, 200));
  centerText(hint, centerX, centerY + 120.f);
  w.draw(hint);
}

void drawRestarting(sf::RenderWindow &w, sf::Font &font) {
  drawOverlay(w, sf::Color(0, 0, 0, 180));
  sf::Text t(font, "Restarting...", 24);
  t.setFillColor(sf::Color::White);
  centerText(t, static_cast<float>(w.getSize().x) / 2.f,
             static_cast<float>(w.getSize().y) / 2.f - 12.f);
  w.draw(t);
}

} // namespace UI
