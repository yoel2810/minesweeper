#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <cstdint>

struct DisplaySettings {
  bool fullscreen = false;
  bool toggleKeyDown = false;
  sf::Vector2u windowedSize{640, 480};

  void toggle(sf::Vector2u currentSize) {
    if (!fullscreen)
      windowedSize = currentSize;
    fullscreen = !fullscreen;
  }

  sf::Vector2u targetSize(sf::Vector2u desktopSize) const {
    return fullscreen ? desktopSize : windowedSize;
  }

  std::uint32_t windowStyle() const {
    return fullscreen ? sf::Style::None : sf::Style::Default;
  }

  sf::State windowState() const { return sf::State::Windowed; }

  bool pressToggleKey() {
    if (toggleKeyDown)
      return false;
    toggleKeyDown = true;
    return true;
  }

  void releaseToggleKey() { toggleKeyDown = false; }
};
