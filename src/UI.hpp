#pragma once
#include "Game.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// "Pocket LCD" theme: a four-shade green reflective screen set in a plastic
// handheld bezel, with magenta rubber buttons for the main actions.
namespace Palette {
inline const sf::Color Darkest{11, 42, 11};
inline const sf::Color Dark{36, 74, 36};
inline const sf::Color Light{104, 129, 11};
inline const sf::Color Lightest{116, 141, 11};
inline const sf::Color Body{201, 195, 182};
inline const sf::Color Bezel{142, 136, 124};
inline const sf::Color Rubber{163, 25, 91};
inline const sf::Color RubberHover{138, 21, 77};
inline const sf::Color RubberShadow{106, 16, 59};
inline const sf::Color RubberText{244, 237, 228};
} // namespace Palette

using CellPos = std::pair<int, int>; // (column, row)

struct Button {
  sf::RectangleShape shape; // hit area and geometry
  sf::Text text;
  sf::Color normal = Palette::Rubber;
  sf::Color hover = Palette::RubberHover;
  sf::Color textColor = Palette::RubberText;
  sf::Color outline = sf::Color::Transparent;
  float outlineThickness = 0.f;
  sf::Color shadow = Palette::RubberShadow; // transparent = no shadow
  float shadowOffset = 4.f;
  float radius = -1.f; // negative = fully rounded pill
  bool hovered = false;

  Button() = delete;
  Button(sf::Font &font, const std::string &str, sf::Vector2f pos,
         sf::Vector2f size, unsigned int charSize = 18);

  bool contains(sf::Vector2i p) const;
  bool contains(sf::Vector2f p) const;
  void setHovered(bool h);
  void setPosition(sf::Vector2f pos);
  void setSize(sf::Vector2f size);
  void draw(sf::RenderWindow &w) const;

private:
  void centerLabel(); // keeps the label centered in the shape
};

struct NumberField {
  sf::RectangleShape shape;
  sf::Text label;
  sf::Text text;
  std::string value = "12";
  bool focused = false;
  bool replaceOnType = false;

  NumberField(sf::Font &font, const std::string &name, sf::Vector2f position);
  bool contains(sf::Vector2i position) const;
  void focus(bool selected);
  void enter(char32_t character);
  void setPosition(sf::Vector2f position);
  void draw(sf::RenderWindow &window);
};

namespace UI {
// Button looks: magenta rubber key, filled LCD key, outlined LCD key.
void styleRubber(Button &b);
void styleScreenPrimary(Button &b);
void styleScreenSecondary(Button &b);

void layoutControls(sf::Vector2u windowSize, Button &startButton,
                    NumberField &columns, NumberField &rows,
                    Button &hudRestartButton, Button &gameOverRestartButton,
                    Button &gameOverMenuButton, Button &gameWonAgainButton,
                    Button &gameWonMenuButton);
// Menu difficulty row (EASY/MEDIUM/HARD/CUSTOM, in Difficulty order) sits
// under the size fields; the MINES % field is the third field in that row.
inline const sf::Vector2f DIFFICULTY_BUTTON_SIZE{92.f, 30.f};
void layoutDifficultyControls(sf::Vector2u windowSize, NumberField &minePercent,
                              std::vector<Button> &difficultyButtons);
// The selected difficulty is a filled key, the others outlined.
void styleDifficultyButtons(std::vector<Button> &difficultyButtons,
                            Difficulty selected);
// Hint key sits left of the solver keys: hint, step, then solve, all
// flanking the HUD restart button.
void layoutSolverControls(sf::Vector2u windowSize, Button &hintButton,
                          Button &stepButton, Button &solveButton);
} // namespace UI

// Every on-screen control, built, styled and laid out together.
struct Controls {
  Button menuStart;
  NumberField columns, rows, mines;
  std::vector<Button> difficulty; // in Difficulty order
  Button hudRestart, hudHint, hudStep, hudSolve;
  Button overRestart, overMenu, wonAgain, wonMenu;

  explicit Controls(sf::Font &font);
  void relayout(sf::Vector2u windowSize);
  // Only the buttons shown in `state` follow the mouse; the rest un-hover,
  // so no button is left highlighted when the screen changes.
  void updateHover(GameState state, sf::Vector2i mouse);
  void focusField(const NumberField *field); // nullptr: none
};

namespace UI {
void drawDevice(sf::RenderWindow &w, bool withHudDivider);
// `marked` gets an outline, e.g. the cell the solver just acted on.
// `hinted` gets a lighter inset outline, the cell the hint key pointed at.
void drawBoard(sf::RenderWindow &w, sf::Font &font, const Game &game,
               std::optional<CellPos> marked = std::nullopt,
               std::optional<CellPos> hinted = std::nullopt);
void drawMenu(sf::RenderWindow &w, sf::Font &font, Controls &controls,
              const std::string &error);
void drawHUD(sf::RenderWindow &w, sf::Font &font, const Game &game,
             const Controls &controls);
void drawGameOver(sf::RenderWindow &w, sf::Font &font, const Game &game,
                  const Controls &controls);
void drawGameWon(sf::RenderWindow &w, sf::Font &font, const Game &game,
                 const Controls &controls);
} // namespace UI
