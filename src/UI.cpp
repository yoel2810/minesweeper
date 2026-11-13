#include "UI.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

// Screen geometry, measured inward from the window edge.
constexpr float BEZEL_INSET = 4.f;  // plastic body visible around the bezel
constexpr float FRAME_INSET = 10.f; // dark frame around the glass
constexpr float SCREEN_INSET = 14.f;
constexpr float HUD_DIVIDER = 3.f;

// Rounded rectangle with independent corner radii (tl, tr, br, bl).
sf::ConvexShape roundedRect(sf::Vector2f pos, sf::Vector2f size, float tl,
                            float tr, float br, float bl) {
  constexpr int steps = 6;
  const float limit = std::min(size.x, size.y) / 2.f;
  const float radii[4] = {std::min(tl, limit), std::min(tr, limit),
                          std::min(br, limit), std::min(bl, limit)};
  const sf::Vector2f centers[4] = {
      {pos.x + radii[0], pos.y + radii[0]},
      {pos.x + size.x - radii[1], pos.y + radii[1]},
      {pos.x + size.x - radii[2], pos.y + size.y - radii[2]},
      {pos.x + radii[3], pos.y + size.y - radii[3]}};
  sf::ConvexShape shape(4 * (steps + 1));
  std::size_t index = 0;
  for (int corner = 0; corner < 4; ++corner) {
    const float start = 3.14159265f * (1.f + corner * 0.5f);
    for (int s = 0; s <= steps; ++s) {
      const float angle = start + (3.14159265f / 2.f) * s / steps;
      shape.setPoint(index++, centers[corner] +
                                  sf::Vector2f(std::cos(angle) * radii[corner],
                                               std::sin(angle) * radii[corner]));
    }
  }
  return shape;
}

sf::ConvexShape roundedRect(sf::Vector2f pos, sf::Vector2f size, float r) {
  return roundedRect(pos, size, r, r, r, r);
}

void fillRect(sf::RenderWindow &w, sf::Vector2f pos, sf::Vector2f size,
              sf::Color color) {
  sf::RectangleShape rect(size);
  rect.setPosition(pos);
  rect.setFillColor(color);
  w.draw(rect);
}

sf::FloatRect screenRect(sf::Vector2u size) {
  return {{SCREEN_INSET, SCREEN_INSET},
          {static_cast<float>(size.x) - 2.f * SCREEN_INSET,
           static_cast<float>(size.y) - 2.f * SCREEN_INSET}};
}

// Places text so its visible glyphs are centered on (centerX, centerY).
void centerTextBoth(sf::Text &t, float centerX, float centerY) {
  const sf::FloatRect b = t.getLocalBounds();
  t.setPosition({std::round(centerX - b.size.x / 2.f - b.position.x),
                 std::round(centerY - b.size.y / 2.f - b.position.y)});
}

void drawText(sf::RenderWindow &w, sf::Font &font, const std::string &str,
              unsigned int size, sf::Color color, float centerX,
              float centerY) {
  sf::Text t(font, str, size);
  t.setFillColor(color);
  centerTextBoth(t, centerX, centerY);
  w.draw(t);
}

// Text with a hard offset copy behind it, like ink on a reflective LCD.
void drawLcdTitle(sf::RenderWindow &w, sf::Font &font, const std::string &str,
                  unsigned int size, float centerX, float centerY) {
  drawText(w, font, str, size, Palette::Light, centerX + 3.f, centerY + 3.f);
  drawText(w, font, str, size, Palette::Darkest, centerX, centerY);
}

// Bitmap art: '#' = ink, 'o' = highlight, anything else = transparent.
constexpr int ART_SIZE = 7;
const char *const MINE_ART[ART_SIZE] = {"...#...", ".#.#.#.", "..###..",
                                        "#.o####", "..###..", ".#.#.#.",
                                        "...#..."};
const char *const FLAG_ART[ART_SIZE] = {"..##...", "..####.", "..#####",
                                        "..#....", "..#....", ".###...",
                                        "#####.."};

void drawArt(sf::RenderWindow &w, const char *const *art, sf::Vector2f cellPos,
             float cellSize, sf::Color ink, sf::Color highlight) {
  const float pixel = std::max(1.f, std::floor(cellSize * 0.75f / ART_SIZE));
  const float artSize = pixel * ART_SIZE;
  const sf::Vector2f origin(std::floor(cellPos.x + (cellSize - artSize) / 2.f),
                            std::floor(cellPos.y + (cellSize - artSize) / 2.f));
  sf::RectangleShape dot({pixel, pixel});
  for (int row = 0; row < ART_SIZE; ++row) {
    for (int col = 0; col < ART_SIZE; ++col) {
      const char c = art[row][col];
      if (c != '#' && c != 'o')
        continue;
      dot.setFillColor(c == '#' ? ink : highlight);
      dot.setPosition({origin.x + col * pixel, origin.y + row * pixel});
      w.draw(dot);
    }
  }
}

// Hidden and revealed cells cover exactly the same square; the dark grid
// lines are the board background showing through the gaps between cells.

// Raised block: lightest top-left edge, dark bottom-right edge. Hovering
// presses the whole block flat in the mid shade.
void drawRaised(sf::RenderWindow &w, sf::Vector2f pos, float size, bool hover) {
  if (hover) {
    fillRect(w, pos, {size, size}, Palette::Dark);
    return;
  }
  const float bevel = std::max(1.f, std::floor(size / 10.f));
  fillRect(w, pos, {size, size}, Palette::Dark);
  fillRect(w, pos, {size - bevel, size - bevel}, Palette::Lightest);
  fillRect(w, pos + sf::Vector2f(bevel, bevel),
           {size - 2.f * bevel, size - 2.f * bevel}, Palette::Light);
}

void drawFlat(sf::RenderWindow &w, sf::Vector2f pos, float size,
              sf::Color fill) {
  fillRect(w, pos, {size, size}, fill);
}

void drawNumber(sf::RenderWindow &w, sf::Font &font, int count,
                sf::Vector2f pos, float size) {
  // Only four shades exist, so the count also changes treatment:
  // 1 = mid ink, 2 = dark ink, 3+ = inverted tile.
  sf::Color ink = count == 1 ? Palette::Dark : Palette::Darkest;
  if (count >= 3) {
    const float inset = std::max(1.f, std::floor(size / 10.f));
    fillRect(w, pos + sf::Vector2f(inset, inset),
             {size - 2.f * inset, size - 2.f * inset}, Palette::Darkest);
    ink = Palette::Lightest;
  }
  const auto charSize =
      static_cast<unsigned int>(std::max(8.f, std::round(size * 1.15f)));
  drawText(w, font, std::to_string(count), charSize, ink, pos.x + size / 2.f,
           pos.y + size / 2.f);
}

std::string formatTime(int seconds) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%02d:%02d", std::min(seconds / 60, 99),
                seconds % 60);
  return buf;
}

int displayedSeconds(const Game &game) {
  if (game.state == PLAYING)
    return static_cast<int>(game.gameClock.getElapsedTime().asSeconds());
  if (game.state == GAME_OVER || game.state == GAME_WON)
    return static_cast<int>(game.elapsedSeconds);
  return 0;
}

// Segment-display readout: unlit "8"s sit behind the lit digits.
void drawReadout(sf::RenderWindow &w, sf::Font &font, const std::string &value,
                 const std::string &caption, float x, float centerY,
                 bool alignRight) {
  constexpr unsigned int digitSize = 44;
  std::string ghost = value;
  for (char &c : ghost)
    if (c >= '0' && c <= '9')
      c = '8';
  sf::Text unlit(font, ghost, digitSize);
  sf::Text lit(font, value, digitSize);
  sf::Text label(font, caption, 18);
  unlit.setFillColor(Palette::Light);
  lit.setFillColor(Palette::Darkest);
  label.setFillColor(Palette::Dark);

  const sf::FloatRect db = lit.getLocalBounds();
  const sf::FloatRect lb = label.getLocalBounds();
  const float gap = 8.f;
  const float total = db.size.x + gap + lb.size.x;
  const float left = alignRight ? x - total : x;
  const sf::Vector2f digitPos(std::round(left - db.position.x),
                              std::round(centerY - db.size.y / 2.f - db.position.y));
  unlit.setPosition(digitPos);
  lit.setPosition(digitPos);
  // Caption sits on the digits' baseline.
  label.setPosition({std::round(left + db.size.x + gap - lb.position.x),
                     std::round(digitPos.y + db.position.y + db.size.y -
                                lb.size.y - lb.position.y)});
  w.draw(unlit);
  w.draw(lit);
  w.draw(label);
}

// Card drawn on the screen: lightest fill, dark border, hard shadow.
void drawPanel(sf::RenderWindow &w, sf::FloatRect rect) {
  fillRect(w, rect.position + sf::Vector2f(8.f, 8.f), rect.size,
           Palette::Darkest);
  sf::RectangleShape panel(rect.size);
  panel.setPosition(rect.position);
  panel.setFillColor(Palette::Lightest);
  panel.setOutlineColor(Palette::Darkest);
  panel.setOutlineThickness(-4.f);
  w.draw(panel);
}

void dimScreen(sf::RenderWindow &w) {
  const sf::FloatRect screen = screenRect(w.getSize());
  sf::Color veil = Palette::Lightest;
  veil.a = 190;
  fillRect(w, screen.position, screen.size, veil);
}

float boardCenterY(sf::Vector2u size) {
  return HEADER_H + (static_cast<float>(size.y) - HEADER_H) / 2.f;
}

void drawEndCard(sf::RenderWindow &w, sf::Font &font, const std::string &title,
                 const std::string &detail, const std::string &hint,
                 Button &primary, Button &secondary) {
  dimScreen(w);
  const auto size = w.getSize();
  const float centerX = static_cast<float>(size.x) / 2.f;
  const float centerY = boardCenterY(size);
  drawPanel(w, {{centerX - 150.f, centerY - 124.f}, {300.f, 252.f}});
  drawLcdTitle(w, font, title, 56, centerX, centerY - 88.f);
  drawText(w, font, detail, 26, Palette::Darkest, centerX, centerY - 40.f);
  primary.draw(w);
  secondary.draw(w);
  drawText(w, font, hint, 18, Palette::Dark, centerX, centerY + 113.f);
}

} // namespace

Button::Button(sf::Font &font, const std::string &str, sf::Vector2f pos,
               sf::Vector2f size, unsigned int charSize)
    : text(font, str, charSize) {
  shape.setPosition(pos);
  shape.setSize(size);
  shape.setFillColor(normal);
  shape.setOutlineThickness(0.f);
  text.setFillColor(textColor);
  // SFML 3: getLocalBounds returns rect with position offset
  sf::FloatRect bounds = text.getLocalBounds();
  float textX = pos.x + (size.x - bounds.size.x) / 2.f - bounds.position.x;
  float textY = pos.y + (size.y - bounds.size.y) / 2.f - bounds.position.y;
  text.setPosition(sf::Vector2f(std::round(textX), std::round(textY)));
}

bool Button::contains(sf::Vector2i p) const {
  return contains(
      sf::Vector2f(static_cast<float>(p.x), static_cast<float>(p.y)));
}

bool Button::contains(sf::Vector2f p) const {
  return shape.getGlobalBounds().contains(p);
}

void Button::setHovered(bool h) {
  hovered = h;
  shape.setFillColor(h ? hover : normal);
}

void Button::setPosition(sf::Vector2f pos) {
  sf::Vector2f delta = pos - shape.getPosition();
  shape.setPosition(pos);
  text.setPosition(text.getPosition() + delta);
}

void Button::setSize(sf::Vector2f size) { shape.setSize(size); }

void Button::draw(sf::RenderWindow &w) const {
  const sf::Vector2f pos = shape.getPosition();
  const sf::Vector2f size = shape.getSize();
  const float r = radius < 0.f ? size.y / 2.f : radius;
  const bool hasShadow = shadow.a > 0;
  // Hovering presses the key halfway into its shadow.
  const sf::Vector2f press =
      hasShadow && hovered ? sf::Vector2f(2.f, 2.f) : sf::Vector2f();

  if (hasShadow) {
    auto back = roundedRect(pos + sf::Vector2f(shadowOffset, shadowOffset),
                            size, r);
    back.setFillColor(shadow);
    w.draw(back);
  }
  auto face = roundedRect(pos + press, size, r);
  face.setFillColor(hovered ? hover : normal);
  face.setOutlineColor(outline);
  face.setOutlineThickness(-outlineThickness);
  w.draw(face);

  sf::Text label = text;
  label.setFillColor(textColor);
  label.move(press);
  w.draw(label);
}

void drawOverlay(sf::RenderWindow &w, sf::Color c) {
  fillRect(w, {0.f, 0.f},
           {static_cast<float>(w.getSize().x), static_cast<float>(w.getSize().y)},
           c);
}

void centerText(sf::Text &t, float centerX, float y) {
  sf::FloatRect b = t.getLocalBounds();
  float x = centerX - (b.size.x / 2.f) - b.position.x;
  float ty = y - b.position.y;
  t.setPosition(sf::Vector2f(x, ty));
}

NumberField::NumberField(sf::Font &font, const std::string &name,
                         sf::Vector2f position)
    : label(font, name, 22), text(font, "12", 34) {
  std::string upper = name;
  for (char &c : upper)
    if (c >= 'a' && c <= 'z')
      c = static_cast<char>(c - 'a' + 'A');
  label.setString(upper);
  shape.setPosition(position);
  shape.setSize({120.f, 38.f});
  shape.setFillColor(Palette::Lightest);
  shape.setOutlineThickness(-3.f);
  label.setPosition({position.x, position.y - 25.f});
  label.setFillColor(Palette::Darkest);
  text.setPosition({position.x + 12.f, position.y - 5.f});
  text.setFillColor(Palette::Darkest);
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
  text.setPosition({position.x + 12.f, position.y - 5.f});
}

void NumberField::draw(sf::RenderWindow &window) {
  static const sf::Clock blinkClock;
  shape.setOutlineColor(focused ? Palette::Darkest : Palette::Dark);
  // A freshly focused field is "selected": typing replaces its value.
  shape.setFillColor(focused && replaceOnType ? Palette::Light
                                              : Palette::Lightest);
  text.setFillColor(focused ? Palette::Darkest : Palette::Dark);
  text.setString(value);
  window.draw(shape);
  window.draw(label);
  window.draw(text);

  const bool cursorOn = blinkClock.getElapsedTime().asMilliseconds() / 500 % 2 == 0;
  if (focused && !replaceOnType && cursorOn) {
    const sf::Vector2f pos = shape.getPosition();
    const sf::FloatRect b = text.getGlobalBounds();
    const float x = value.empty() ? pos.x + 12.f : b.position.x + b.size.x + 3.f;
    fillRect(window, {std::round(x), pos.y + 8.f}, {10.f, 22.f},
             Palette::Darkest);
  }
}

namespace UI {

void styleRubber(Button &b) {
  b.normal = Palette::Rubber;
  b.hover = Palette::RubberHover;
  b.textColor = Palette::RubberText;
  b.outline = sf::Color::Transparent;
  b.outlineThickness = 0.f;
  b.shadow = Palette::RubberShadow;
  b.radius = -1.f;
  b.setHovered(b.hovered);
}

void styleScreenPrimary(Button &b) {
  b.normal = Palette::Darkest;
  b.hover = Palette::Dark;
  b.textColor = Palette::Lightest;
  b.outline = Palette::Darkest;
  b.outlineThickness = 2.f;
  b.shadow = sf::Color::Transparent;
  b.radius = 0.f;
  b.setHovered(b.hovered);
}

void styleScreenSecondary(Button &b) {
  b.normal = Palette::Lightest;
  b.hover = Palette::Light;
  b.textColor = Palette::Darkest;
  b.outline = Palette::Darkest;
  b.outlineThickness = 2.f;
  b.shadow = sf::Color::Transparent;
  b.radius = 0.f;
  b.setHovered(b.hovered);
}

void layoutControls(sf::Vector2u windowSize, Button &startButton,
                    NumberField &columns, NumberField &rows,
                    Button &hudRestartButton, Button &gameOverRestartButton,
                    Button &gameOverMenuButton, Button &gameWonAgainButton,
                    Button &gameWonMenuButton) {
  const float centerX = static_cast<float>(windowSize.x) / 2.f;
  const float centerY = boardCenterY(windowSize);
  startButton.setPosition({centerX - 100.f, centerY + 32.f});
  columns.setPosition({centerX - 200.f, centerY - 84.f});
  rows.setPosition({centerX - 60.f, centerY - 84.f});
  hudRestartButton.setPosition({centerX - 16.f, 20.f});
  gameOverRestartButton.setPosition({centerX - 100.f, centerY + 10.f});
  gameOverMenuButton.setPosition({centerX - 100.f, centerY + 62.f});
  gameWonAgainButton.setPosition({centerX - 100.f, centerY + 10.f});
  gameWonMenuButton.setPosition({centerX - 100.f, centerY + 62.f});
}

void layoutDifficultyControls(sf::Vector2u windowSize, NumberField &minePercent,
                              std::vector<Button> &difficultyButtons) {
  const float centerX = static_cast<float>(windowSize.x) / 2.f;
  const float centerY = boardCenterY(windowSize);
  minePercent.setPosition({centerX + 80.f, centerY - 84.f});
  const float width = DIFFICULTY_BUTTON_SIZE.x, gap = 8.f;
  const float rowWidth = difficultyButtons.size() * (width + gap) - gap;
  for (std::size_t i = 0; i < difficultyButtons.size(); ++i)
    difficultyButtons[i].setPosition(
        {centerX - rowWidth / 2.f + i * (width + gap), centerY - 32.f});
}

void styleDifficultyButtons(std::vector<Button> &difficultyButtons,
                            Difficulty selected) {
  for (std::size_t i = 0; i < difficultyButtons.size(); ++i) {
    if (static_cast<int>(i) == static_cast<int>(selected))
      styleScreenPrimary(difficultyButtons[i]);
    else
      styleScreenSecondary(difficultyButtons[i]);
  }
}

void layoutSolverControls(sf::Vector2u windowSize, Button &stepButton,
                          Button &solveButton) {
  const float centerX = static_cast<float>(windowSize.x) / 2.f;
  stepButton.setPosition({centerX - 56.f, 20.f});
  solveButton.setPosition({centerX + 24.f, 20.f});
}

void drawDevice(sf::RenderWindow &w, bool withHudDivider) {
  const auto size = w.getSize();
  const sf::Vector2f full(static_cast<float>(size.x), static_cast<float>(size.y));

  // Plastic bezel with the signature large bottom-right corner.
  auto bezel = roundedRect({BEZEL_INSET, BEZEL_INSET},
                           full - sf::Vector2f(2.f, 2.f) * BEZEL_INSET, 10.f,
                           10.f, 30.f, 10.f);
  bezel.setFillColor(Palette::Bezel);
  w.draw(bezel);
  auto frame = roundedRect({FRAME_INSET, FRAME_INSET},
                           full - sf::Vector2f(2.f, 2.f) * FRAME_INSET, 4.f,
                           4.f, 20.f, 4.f);
  frame.setFillColor(Palette::Darkest);
  w.draw(frame);

  // Glass, with a recessed edge: shadow top-left, glare bottom-right.
  const sf::FloatRect s = screenRect(size);
  const float edge = 3.f;
  fillRect(w, s.position, s.size, Palette::Lightest);
  fillRect(w, s.position, {s.size.x, edge}, Palette::Dark);
  fillRect(w, s.position, {edge, s.size.y}, Palette::Dark);
  fillRect(w, {s.position.x + edge, s.position.y + s.size.y - edge},
           {s.size.x - edge, edge}, Palette::Light);
  fillRect(w, {s.position.x + s.size.x - edge, s.position.y + edge},
           {edge, s.size.y - edge}, Palette::Light);

  if (withHudDivider)
    fillRect(w, {s.position.x + 12.f, HEADER_H - HUD_DIVIDER - 2.f},
             {s.size.x - 24.f, HUD_DIVIDER}, Palette::Darkest);
}

void drawBoard(sf::RenderWindow &w, sf::Font &font, const Game &game) {
  const auto size = w.getSize();
  const auto layout =
      BoardLayout::fit(game.columns(), game.rows(), size.x, size.y);
  const float cell = static_cast<float>(layout.cellSize);
  // Each cell leaves a gap on its right and bottom for the grid line, so
  // the frame is thinner on those sides to keep all four borders equal.
  const float gap = std::max(1.f, std::floor(cell / 15.f));
  const float inner = cell - gap;
  const float frame = 3.f;
  fillRect(w, {layout.x - frame, layout.y - frame},
           {layout.columns * cell + 2.f * frame - gap,
            layout.rows * cell + 2.f * frame - gap},
           Palette::Darkest);

  std::optional<std::pair<int, int>> hovered;
  if (game.state == PLAYING) {
    const auto mouse = sf::Mouse::getPosition(w);
    hovered = layout.cellAt(mouse.x, mouse.y);
  }

  for (int i = 0; i < game.columns(); i++) {
    for (int j = 0; j < game.rows(); j++) {
      const Cell &c = game.grid[i][j];
      const sf::Vector2f pos(static_cast<float>(layout.x) + i * cell,
                             static_cast<float>(layout.y) + j * cell);
      const bool exploded = i == game.explodedColumn && j == game.explodedRow;
      if (c.isRevealed && c.isMine) {
        drawFlat(w, pos, inner, exploded ? Palette::Darkest : Palette::Lightest);
        drawArt(w, MINE_ART, pos, inner,
                exploded ? Palette::Lightest : Palette::Darkest,
                exploded ? Palette::Darkest : Palette::Lightest);
      } else if (c.isRevealed && !c.isFlag) {
        drawFlat(w, pos, inner, Palette::Lightest);
        if (c.minesCount > 0)
          drawNumber(w, font, c.minesCount, pos, inner);
      } else {
        const bool hover =
            !c.isRevealed && hovered && hovered->first == i && hovered->second == j;
        drawRaised(w, pos, inner, hover);
        if (c.isFlag)
          drawArt(w, FLAG_ART, pos, inner, Palette::Darkest, Palette::Darkest);
      }
    }
  }
}

void drawMenu(sf::RenderWindow &w, sf::Font &font, Button &startButton,
              NumberField &columns, NumberField &rows,
              NumberField &minePercent,
              const std::vector<Button> &difficultyButtons,
              const std::string &error) {
  const auto size = w.getSize();
  const float centerX = static_cast<float>(size.x) / 2.f;
  const float centerY = boardCenterY(size);

  drawLcdTitle(w, font, "MINESWEEPER", 72, centerX, centerY - 142.f);

  columns.draw(w);
  rows.draw(w);
  minePercent.draw(w);
  for (const Button &button : difficultyButtons)
    button.draw(w);

  if (error.empty()) {
    drawText(w, font, "SIZE 4-30  MINES 5-50%  ENTER: START", 18,
             Palette::Dark, centerX, centerY + 15.f);
  } else {
    // No red on an LCD: errors are shown as an inverted strip.
    sf::Text msg(font, error, 18);
    msg.setFillColor(Palette::Lightest);
    centerTextBoth(msg, centerX, centerY + 15.f);
    const sf::FloatRect b = msg.getGlobalBounds();
    fillRect(w, {b.position.x - 6.f, b.position.y - 4.f},
             {b.size.x + 12.f, b.size.y + 8.f}, Palette::Darkest);
    w.draw(msg);
  }

  startButton.draw(w);

  drawText(w, font, "LEFT: REVEAL  RIGHT: FLAG  R: RESTART", 16,
           Palette::Dark, centerX, centerY + 102.f);
  drawText(w, font, "ESC: MENU  F11: FULLSCREEN  FIRST CLICK IS SAFE", 16,
           Palette::Dark, centerX, centerY + 120.f);
}

void drawHUD(sf::RenderWindow &w, sf::Font &font, Game &game,
             Button &restartButton, Button &stepButton, Button &solveButton) {
  const float width = static_cast<float>(w.getSize().x);
  const float centerY = SCREEN_INSET + (HEADER_H - HUD_DIVIDER - 2.f - SCREEN_INSET) / 2.f;
  const int remaining = std::max(0, game.mineCount() - game.flagCount());
  char mines[8];
  std::snprintf(mines, sizeof(mines), "%03d", std::min(remaining, 999));

  drawReadout(w, font, mines, "MINES", SCREEN_INSET + 18.f, centerY, false);
  drawReadout(w, font, formatTime(displayedSeconds(game)), "TIME",
              width - SCREEN_INSET - 18.f, centerY, true);
  restartButton.draw(w);
  stepButton.draw(w);
  solveButton.draw(w);
}

void drawGameOver(sf::RenderWindow &w, sf::Font &font, Game &game,
                  Button &restartButton, Button &menuButton) {
  drawEndCard(w, font, "GAME OVER",
              "TIME " + formatTime(displayedSeconds(game)),
              "R: RESTART  ESC: MENU", restartButton, menuButton);
}

void drawGameWon(sf::RenderWindow &w, sf::Font &font, Game &game,
                 Button &againButton, Button &menuButton) {
  drawEndCard(w, font, "YOU WIN!", "TIME " + formatTime(displayedSeconds(game)),
              "R: PLAY AGAIN  ESC: MENU", againButton, menuButton);
}

void drawRestarting(sf::RenderWindow &w, sf::Font &font) {
  dimScreen(w);
  drawLcdTitle(w, font, "RESTARTING...", 40,
               static_cast<float>(w.getSize().x) / 2.f,
               static_cast<float>(w.getSize().y) / 2.f);
}

} // namespace UI
