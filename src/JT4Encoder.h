#pragma once
#include <Arduino.h>

// JT4 encoder: thin wrapper around Etherkit JTEncode's jt4_encode(), which
// does the actual WSJT-family message packing and convolutional coding.
// JT4 is 4-tone FSK (hence the name), with seven submodes A-G that only
// change tone spacing -- symbol count and symbol period are constant.
namespace JT4 {

constexpr uint16_t SYMBOL_COUNT = 207; // JT4_SYMBOL_COUNT in JTEncode.h, fixed for every submode
constexpr uint8_t TONE_COUNT = 4;
constexpr float SYMBOL_PERIOD_MS = 2520.0f * 1000.0f / 11025.0f; // 228.571ms (4.375 baud), fixed for every submode (~47.3s/207 symbols per 1-min slot)

// Tone-spacing submode. Values verified against the public JT4 spacing
// table (spacing does NOT cleanly double at every step -- C to D is a
// factor of 2.25, not 2, which is a real, deliberate irregularity in the
// original spec, not a typo).
enum class Submode : uint8_t { A, B, C, D, E, F, G };
constexpr Submode DEFAULT_SUBMODE = Submode::G; // matches this beacon's previous hardcoded config

// Encodes `message` into `tones[0..206]` (values 0..3). `message` is passed
// to JTEncode::jt4_encode() as-is (it does its own message validation); it
// must be 13 characters or fewer. The encoded symbol sequence is identical
// for every submode -- only tone spacing changes what it means on the air.
bool encode(const char *message, uint8_t tones[SYMBOL_COUNT]);

// Tone spacing in Hz (at freqMulti=1) for a given submode.
float toneSpacingHz(Submode submode);

} // namespace JT4
