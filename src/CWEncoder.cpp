#include "CWEncoder.h"
#include <CWLibrary.hpp>

namespace {
// CWLibrary's key-up/key-down callbacks take no arguments, but the
// SetFrequencyFn this module (and BeaconModes) is handed takes a double.
// These module-level statics bridge the two for the duration of one
// transmit() call -- CW::transmit() is not reentrant/thread-safe across
// concurrent calls, same as the rest of this codebase's single-threaded-
// per-core transmit path.
CW::SetFrequencyFn g_setFrequency = nullptr;
double g_markHz = 0;
double g_spaceHz = 0;

void keyDownThunk() { g_setFrequency(g_markHz); }
void keyUpThunk() { g_setFrequency(g_spaceHz); }
} // namespace

namespace CW {

void transmit(const char *message, double markHz, float freqMulti,
              SetFrequencyFn setFrequency, uint8_t wpm, float spaceShiftHz) {
  if (!message || !setFrequency) return;

  g_setFrequency = setFrequency;
  g_markHz = markHz;
  g_spaceHz = markHz - spaceShiftHz / freqMulti;

  CWLibrary cw(wpm, keyDownThunk, keyUpThunk);
  cw.sendMessage(const_cast<char *>(message)); // CWLibrary never writes through this pointer, just reads it
}

} // namespace CW
