#include "BeaconModes.h"

uint16_t BeaconModes::encode(BeaconMode mode, const char *message, uint8_t *tones) {
  bool ok = false;
  switch (mode) {
    case BeaconMode::CW: return 0; // see class doc comment: use transmit() for CW
    case BeaconMode::Q65: ok = Q65::encode(message, tones); break;
    case BeaconMode::PI4: ok = PI4::encode(message, tones); break;
    case BeaconMode::JT4: ok = JT4::encode(message, tones); break;
  }
  return ok ? maxSymbols(mode) : 0;
}

uint16_t BeaconModes::maxSymbols(BeaconMode mode) {
  switch (mode) {
    case BeaconMode::CW: return 0;
    case BeaconMode::Q65: return Q65::SYMBOL_COUNT;
    case BeaconMode::PI4: return PI4::SYMBOL_COUNT;
    case BeaconMode::JT4: return JT4::SYMBOL_COUNT;
  }
  return 0;
}

uint8_t BeaconModes::toneCount(BeaconMode mode) {
  switch (mode) {
    case BeaconMode::CW: return 0;
    case BeaconMode::Q65: return Q65::TONE_COUNT;
    case BeaconMode::PI4: return PI4::TONE_COUNT;
    case BeaconMode::JT4: return JT4::TONE_COUNT;
  }
  return 0;
}

float BeaconModes::symbolPeriodMs(BeaconMode mode, Q65Submode q65sub) {
  switch (mode) {
    case BeaconMode::CW: return 0;
    case BeaconMode::Q65: return Q65::symbolPeriodMs(q65sub.duration);
    case BeaconMode::PI4: return PI4::SYMBOL_PERIOD_MS;
    case BeaconMode::JT4: return JT4::SYMBOL_PERIOD_MS;
  }
  return 0;
}

float BeaconModes::toneSpacingHz(BeaconMode mode, float freqMulti, Q65Submode q65sub, JT4Submode jt4sub) {
  float base;
  switch (mode) {
    case BeaconMode::CW: return 0;
    case BeaconMode::Q65: base = Q65::toneSpacingHz(q65sub.duration, q65sub.bandwidth); break;
    case BeaconMode::PI4: base = PI4::TONE_SPACING_HZ; break;
    case BeaconMode::JT4: base = JT4::toneSpacingHz(jt4sub.submode); break;
    default: return 0;
  }
  return base / freqMulti;
}

bool BeaconModes::transmit(BeaconMode mode, const char *message, double markHz,
                            float freqMulti, SetFrequencyFn setFrequency,
                            Q65Submode q65sub, JT4Submode jt4sub, CWSubmode cwsub) {
  if (mode == BeaconMode::CW) {
    CW::transmit(message, markHz, freqMulti, setFrequency, cwsub.wpm, cwsub.spaceShiftHz);
    return true;
  }

  uint8_t tones[MAX_SYMBOLS];
  uint16_t n = encode(mode, message, tones);
  if (n == 0) return false;

  double spacing = toneSpacingHz(mode, freqMulti, q65sub, jt4sub);
  uint32_t periodMs = (uint32_t)(symbolPeriodMs(mode, q65sub) + 0.5f);
  for (uint16_t i = 0; i < n; i++) {
    setFrequency(markHz + tones[i] * spacing);
    delay(periodMs);
  }
  return true;
}
