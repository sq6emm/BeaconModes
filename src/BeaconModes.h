#pragma once
#include <Arduino.h>
#include "Q65Encoder.h"
#include "PI4Encoder.h"
#include "JT4Encoder.h"
#include "CWEncoder.h"

// Unified mode+message interface over CWEncoder / Q65Encoder / PI4Encoder /
// JT4Encoder: pick a BeaconMode, hand it a message (and, optionally, a
// submode), get back a tone-index sequence (or have it drive a synthesizer
// directly via transmit()).
//
// CW is the odd one out: Morse carries information in the *duration* of
// marks and spaces, not by picking one of a fixed number of tones at a
// fixed symbol period the way Q65/PI4/JT4 do -- so encode()/maxSymbols()/
// toneCount()/symbolPeriodMs()/toneSpacingHz() don't apply to it and return
// 0/false for BeaconMode::CW (see each function's doc comment). transmit()
// fully supports CW; it's the one call every mode is guaranteed to work
// through, which is the point of this class.
enum class BeaconMode : uint8_t { CW, Q65, PI4, JT4 };

// Submode selectors, bundled per-mode so the unified calls below take one
// argument regardless of mode. Defaults reproduce this library's original
// single-config behavior (Q65-60D, JT4G, 12 WPM CW) -- the ones already on
// the air -- so existing calls that don't pass these need no changes. PI4
// has only one configuration, so it ignores all of these.
//
// Each has an explicit constructor (not just default member initializers)
// so `Q65Submode{a, b}`-style construction works under the -std=gnu++11
// this toolchain builds with -- a struct with default member initializers
// is not an aggregate before C++14, so plain brace-init would otherwise
// fail to compile.
struct Q65Submode {
  Q65::Duration duration = Q65::DEFAULT_DURATION;
  Q65::Bandwidth bandwidth = Q65::DEFAULT_BANDWIDTH;
  Q65Submode() = default;
  Q65Submode(Q65::Duration d, Q65::Bandwidth b) : duration(d), bandwidth(b) {}
};
struct JT4Submode {
  JT4::Submode submode = JT4::DEFAULT_SUBMODE;
  JT4Submode() = default;
  JT4Submode(JT4::Submode s) : submode(s) {}
};
struct CWSubmode {
  uint8_t wpm = CW::DEFAULT_WPM;
  float spaceShiftHz = CW::DEFAULT_SPACE_SHIFT_HZ;
  CWSubmode() = default;
  CWSubmode(uint8_t w, float s) : wpm(w), spaceShiftHz(s) {}
};

class BeaconModes {
public:
  // Largest symbol count across the FSK modes (CW has no fixed symbol
  // count) -- size a caller-allocated tones[] buffer to this if it needs to
  // be reused across modes.
  static constexpr uint16_t MAX_SYMBOLS = JT4::SYMBOL_COUNT; // 207

  // Encodes `message` for `mode` into `tones` (caller-allocated, at least
  // maxSymbols(mode) bytes, values 0..toneCount(mode)-1). Returns the
  // number of symbols written, or 0 if this mode's encoder rejected the
  // message (e.g. a PI4 message with an unsupported character), OR if
  // mode is CW (see class doc comment -- use transmit() for CW). The
  // encoded symbol sequence does not depend on submode.
  static uint16_t encode(BeaconMode mode, const char *message, uint8_t *tones);

  static uint16_t maxSymbols(BeaconMode mode); // 0 for CW
  static uint8_t toneCount(BeaconMode mode);   // 0 for CW
  static float symbolPeriodMs(BeaconMode mode, Q65Submode q65sub = {}); // 0 for CW
  // Tone spacing in Hz at the fundamental (pre-multiplier) synth frequency,
  // already divided by freqMulti so the actual on-air spacing stays
  // constant across bands -- the same convention MGMBeacon.ino already uses
  // for Q65 (DF = 13.3333334/freqMulti). 0 for CW (see CWSubmode.spaceShiftHz
  // for CW's analogous constant).
  static float toneSpacingHz(BeaconMode mode, float freqMulti = 1.0f,
                              Q65Submode q65sub = {}, JT4Submode jt4sub = {});

  // One-shot transmit: for CW, sends `message` in Morse (see CWEncoder.h);
  // for the FSK modes, encodes the message then for each symbol calls
  // setFrequency(markHz + toneIndex*spacing) followed by
  // delay(symbolPeriodMs(mode, q65sub)). Mirrors the SetFrequency-callback
  // style CWLibrary/ADF4157 already use elsewhere in this codebase. Leaves
  // the synth wherever the last symbol/character left it -- callers that
  // need to restore the resting/carrier frequency should call
  // setFrequency(markHz) themselves afterward. Returns false (nothing
  // transmitted) if encoding failed (FSK modes only; CW always "succeeds"
  // since it has no message-format constraints beyond what CWLibrary's own
  // Morse table covers).
  typedef void (*SetFrequencyFn)(double freqHz);
  static bool transmit(BeaconMode mode, const char *message, double markHz,
                        float freqMulti, SetFrequencyFn setFrequency,
                        Q65Submode q65sub = {}, JT4Submode jt4sub = {},
                        CWSubmode cwsub = {});
};
