#include "Audio.hpp"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace {
constexpr unsigned SAMPLE_RATE = 44100;
using Samples = std::vector<std::int16_t>;

// Appends a tone sweeping startHz -> endHz over `ms`, with a linear fade-out.
// `noise` > 0 blends in white noise (0..1); `square` selects square vs sine.
void addTone(Samples &out, float startHz, float endHz, int ms, float volume,
             bool square = true, float noise = 0.f) {
  const int count = static_cast<int>(SAMPLE_RATE * ms / 1000);
  float phase = 0.f;
  for (int i = 0; i < count; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(count);
    const float hz = startHz + (endHz - startHz) * t;
    phase += hz / SAMPLE_RATE;
    phase -= std::floor(phase);
    float wave = square ? (phase < 0.5f ? 1.f : -1.f)
                        : std::sin(phase * 6.2831853f);
    if (noise > 0.f) {
      const float n = static_cast<float>(std::rand()) / RAND_MAX * 2.f - 1.f;
      wave = wave * (1.f - noise) + n * noise;
    }
    const float envelope = 1.f - t;
    out.push_back(static_cast<std::int16_t>(wave * envelope * volume * 32767.f));
  }
}

void addSilence(Samples &out, int ms) {
  out.insert(out.end(), SAMPLE_RATE * ms / 1000, 0);
}

Samples build(Sfx sfx) {
  Samples s;
  switch (sfx) {
  case Sfx::Reveal:
    addTone(s, 880.f, 700.f, 40, 0.18f);
    break;
  case Sfx::Flood:
    addTone(s, 500.f, 1100.f, 140, 0.18f);
    break;
  case Sfx::Flag:
    addTone(s, 660.f, 660.f, 35, 0.2f);
    addTone(s, 990.f, 990.f, 55, 0.2f);
    break;
  case Sfx::Unflag:
    addTone(s, 990.f, 990.f, 35, 0.2f);
    addTone(s, 660.f, 660.f, 55, 0.2f);
    break;
  case Sfx::Mine:
    addTone(s, 160.f, 40.f, 600, 0.5f, true, 0.7f);
    break;
  case Sfx::Win:
    for (float hz : {523.f, 659.f, 784.f}) {
      addTone(s, hz, hz, 110, 0.2f);
      addSilence(s, 15);
    }
    addTone(s, 1047.f, 1047.f, 400, 0.2f);
    break;
  case Sfx::Click:
    addTone(s, 1200.f, 900.f, 25, 0.15f);
    break;
  case Sfx::Hint:
    addTone(s, 1318.f, 1318.f, 60, 0.15f, false);
    addTone(s, 1760.f, 1760.f, 120, 0.15f, false);
    break;
  case Sfx::NoHint:
    addTone(s, 220.f, 180.f, 90, 0.2f);
    addSilence(s, 30);
    addTone(s, 180.f, 140.f, 140, 0.2f);
    break;
  default:
    break;
  }
  return s;
}
} // namespace

Audio::Audio() {
  for (std::size_t i = 0; i < COUNT; ++i) {
    const Samples samples = build(static_cast<Sfx>(i));
    if (samples.empty() ||
        !buffers[i].loadFromSamples(samples.data(), samples.size(), 1,
                                    SAMPLE_RATE, {sf::SoundChannel::Mono}))
      continue;
    sounds[i] = std::make_unique<sf::Sound>(buffers[i]);
  }
}

void Audio::play(Sfx sfx) {
  if (muted)
    return;
  if (auto &sound = sounds[static_cast<std::size_t>(sfx)])
    sound->play();
}
