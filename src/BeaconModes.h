#pragma once
#include <Arduino.h>
#include "Q65Encoder.h"
#include "PI4Encoder.h"
#include "JT4Encoder.h"

// Unified mode+message interface over Q65Encoder / PI4Encoder / JT4Encoder:
// pick a BeaconMode, hand it a message (and, optionally, a submode), get
// back a tone-index sequence (or have it drive a synthesizer directly via
// transmit()).
enum class BeaconMode : uint8_t { Q65, PI4, JT4 };

// Submode selectors, bundled per-mode so the unified calls below take one
// argument regardless of mode. Defaults reproduce this library's original
// single-config behavior (Q65-60D, JT4G) -- the ones already on the air --
// so existing calls that don't pass these need no changes. PI4 has only
// one configuration, so it ignores both.
struct Q65Submode {
  Q65::Duration duration = Q65::DEFAULT_DURATION;
  Q65::Bandwidth bandwidth = Q65::DEFAULT_BANDWIDTH;
};
struct JT4Submode {
  JT4::Submode submode = JT4::DEFAULT_SUBMODE;
};

class BeaconModes {
public:
  // Largest symbol count across all modes -- size a caller-allocated tones[]
  // buffer to this if it needs to be reused across modes.
  static constexpr uint16_t MAX_SYMBOLS = JT4::SYMBOL_COUNT; // 207

  // Encodes `message` for `mode` into `tones` (caller-allocated, at least
  // maxSymbols(mode) bytes, values 0..toneCount(mode)-1). Returns the
  // number of symbols written, or 0 if this mode's encoder rejected the
  // message (e.g. a PI4 message with an unsupported character). The
  // encoded symbol sequence does not depend on submode.
  static uint16_t encode(BeaconMode mode, const char *message, uint8_t *tones);

  static uint16_t maxSymbols(BeaconMode mode);
  static uint8_t toneCount(BeaconMode mode);
  static float symbolPeriodMs(BeaconMode mode, Q65Submode q65sub = {});
  // Tone spacing in Hz at the fundamental (pre-multiplier) synth frequency,
  // already divided by freqMulti so the actual on-air spacing stays
  // constant across bands -- the same convention MGMBeacon.ino already uses
  // for Q65 (DF = 13.3333334/freqMulti).
  static float toneSpacingHz(BeaconMode mode, float freqMulti = 1.0f,
                              Q65Submode q65sub = {}, JT4Submode jt4sub = {});

  // One-shot transmit: encodes the message, then for each symbol calls
  // setFrequency(markHz + toneIndex*spacing) followed by
  // delay(symbolPeriodMs(mode, q65sub)). Mirrors the SetFrequency-callback
  // style CWLibrary/ADF4157 already use elsewhere in this codebase. Leaves
  // the synth wherever the last symbol left it -- callers that need to
  // restore the resting/carrier frequency should call setFrequency(markHz)
  // themselves afterward. Returns false (nothing transmitted) if encoding
  // failed.
  typedef void (*SetFrequencyFn)(double freqHz);
  static bool transmit(BeaconMode mode, const char *message, double markHz,
                        float freqMulti, SetFrequencyFn setFrequency,
                        Q65Submode q65sub = {}, JT4Submode jt4sub = {});
};
