#pragma once
#include <Arduino.h>

// PI4 encoder, ported from Bo Hansen OZ2M's own PI4Ino reference
// implementation (as published in la1k/la1k-cw-beacon's BeaconPI4CW.ino,
// https://github.com/la1k/la1k-cw-beacon -- the same PI4Ino project this
// beacon's sketch header already credits). PI4 is a 4-tone FSK beacon mode
// "based on JT4": an 8-character message, base-38 source encoding, a
// rate-1/2 convolutional code, a bit-reversal interleaver, and a fixed
// 146-symbol sync vector added in to pick the transmitted tone.
namespace PI4 {

constexpr uint16_t SYMBOL_COUNT = 146;
constexpr uint8_t TONE_COUNT = 4; // tone values 0..3
constexpr uint16_t SYMBOL_PERIOD_MS = 167; // 166.667ms, per OZ2M's reference
constexpr float TONE_SPACING_HZ = 234.375f; // divide by freqMulti for the actual synth output
constexpr uint8_t MAX_MESSAGE_LEN = 8;

// Encodes `message` (up to 8 characters from "0-9A-Z /", space-padded if
// shorter) into `tones[0..145]` (values 0..3). Returns false if `message`
// (uppercased) contains a character outside that set.
bool encode(const char *message, uint8_t tones[SYMBOL_COUNT]);

} // namespace PI4
