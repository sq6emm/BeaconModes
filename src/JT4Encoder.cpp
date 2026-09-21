#include "JT4Encoder.h"
#include <JTEncode.h>
#include <cstring>

namespace JT4 {

bool encode(const char *message, uint8_t tones[SYMBOL_COUNT]) {
  if (!message) return false;
  // JTEncode::jt4_encode() copies into a fixed 14-byte internal buffer with
  // strcpy() and no length check; guard against overrunning it here.
  if (strlen(message) > 13) return false;
  static JTEncode jtencode;
  jtencode.jt4_encode(message, tones);
  return true;
}

float toneSpacingHz(Submode submode) {
  static const float spacing[7] = {4.375f, 8.75f, 17.5f, 39.375f, 78.75f, 157.5f, 315.0f}; // A..G
  return spacing[(int)submode];
}

} // namespace JT4
