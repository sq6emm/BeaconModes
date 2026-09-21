#pragma once
#include <Arduino.h>

// CW (Morse) transmit, wrapping the existing CWLibrary
// (github.com/bobboteck/CWLibrary, already used by this codebase).
//
// CW doesn't fit the other three modes' encode()->tones[]/toneCount()/
// toneSpacingHz()/symbolPeriodMs() shape: Morse carries information in the
// *duration* of marks and spaces (dots, dashes, inter-character and
// inter-word gaps), not by picking one of a fixed number of tones at a
// fixed symbol period. So there is no CW::encode() here -- transmit() is
// the only entry point, called directly by BeaconModes::transmit(BeaconMode::CW, ...).
namespace CW {

constexpr uint8_t DEFAULT_WPM = 12;
constexpr float DEFAULT_SPACE_SHIFT_HZ = 400.0f; // key-up tone offset below markHz

typedef void (*SetFrequencyFn)(double freqHz);

// Sends `message` in Morse at `wpm`: setFrequency(markHz) while keyed down
// (mark/dash/dot), setFrequency(markHz - spaceShiftHz/freqMulti) while
// keyed up. This is a two-tone frequency shift rather than true on/off
// keying, matching how this codebase's beacons avoid the key-click a hard
// carrier drop causes on a PLL synthesizer -- spaceShiftHz is divided by
// freqMulti so the actual on-air shift stays constant across bands, same
// convention as the other modes' tone spacing.
void transmit(const char *message, double markHz, float freqMulti,
              SetFrequencyFn setFrequency, uint8_t wpm = DEFAULT_WPM,
              float spaceShiftHz = DEFAULT_SPACE_SHIFT_HZ);

} // namespace CW
