#pragma once

#include <SFML/Audio.hpp>
#include <array>
#include <memory>

// Retro sound effects synthesized in code (square waves and noise), so the
// game ships no audio files. If no audio device exists the game stays silent.
enum class Sfx {
  Reveal,
  Flood,
  Flag,
  Unflag,
  Mine,
  Win,
  Click,
  Hint,
  NoHint,
  Count
};

class Audio {
public:
  Audio();
  void play(Sfx sfx);
  void toggleMute() { muted = !muted; }
  bool isMuted() const { return muted; }

private:
  static constexpr std::size_t COUNT = static_cast<std::size_t>(Sfx::Count);
  std::array<sf::SoundBuffer, COUNT> buffers;
  std::array<std::unique_ptr<sf::Sound>, COUNT> sounds;
  bool muted = false;
};
