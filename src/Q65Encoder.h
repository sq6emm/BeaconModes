#pragma once
#include <Arduino.h>

// Q65 encoder, ported bit-for-bit from WSJT-X's own source
// (lib/qra/q65/q65_encoding_modules.f90 and lib/77bit/packjt77.f90,
// https://github.com/WSJTX/wsjtx) and verified against three known-good
// message -> tone-sequence pairs pulled from a real Q65 beacon's hardcoded
// tables (see BeaconModesSelfTest example).
//
// Only two of pack77's real message-type paths are implemented, since a
// beacon only ever needs these:
//  - Type 1 standard message: "CALL1 CALL2 [GRID4]", including the special
//    tokens "DE ", "CQ ", "QRZ " as CALL1 (e.g. "DE SR3LES JO81").
//  - Free-text fallback (base-42 packing of up to 13 characters) for
//    anything that doesn't fit the above (e.g. a lone callsign+grid with
//    no DE/CQ/QRZ prefix, since a 4-character grid square doesn't parse as
//    a second callsign).
// WSJT-X's contest-exchange and DXpedition-mode message types are NOT
// implemented; messages that would need them fall through to free text
// instead, same as any other non-matching message.
namespace Q65 {

constexpr uint16_t SYMBOL_COUNT = 85; // fixed -- same for every T/bandwidth submode
constexpr uint8_t TONE_COUNT = 65;    // tone values 0..64; 0 doubles as sync and the "mark"/rest tone

// Encodes `message` into `tones[0..84]` (values 0..64). Returns true on
// success. `message` is uppercased internally; it is not modified. The
// encoded symbol sequence is identical for every submode below -- only the
// symbol period and tone spacing change what it means on the air.
bool encode(const char *message, uint8_t tones[SYMBOL_COUNT]);

// T/R period, in seconds: how long one 85-symbol transmission is spread
// over. Matches WSJT-X's own lib/q65params.f90 (ntrp/nsps tables).
enum class Duration : uint8_t { T15, T30, T60, T120, T300 };

// Tone-spacing class: each step doubles the spacing (wider = more
// Doppler/fading tolerant, at the cost of bandwidth and SNR threshold).
// Matches the "letter" in mode names like "Q65-60D" (this beacon's
// previous hardcoded config, and this library's default).
enum class Bandwidth : uint8_t { A, B, C, D, E };

constexpr Duration DEFAULT_DURATION = Duration::T60;
constexpr Bandwidth DEFAULT_BANDWIDTH = Bandwidth::D;

// Symbol period in milliseconds for a given T/R duration (constant across
// bandwidth classes -- only the duration affects timing).
float symbolPeriodMs(Duration duration);

// Tone spacing in Hz (at freqMulti=1) for a given duration+bandwidth pair.
float toneSpacingHz(Duration duration, Bandwidth bandwidth);

} // namespace Q65
