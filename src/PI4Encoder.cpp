#include "PI4Encoder.h"
#include <cstring>
#include <cctype>

namespace {

// PI4's fixed 8-char base-38 alphabet (from OZ2M's PI4Ino reference).
const char PI4Chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ /";

// Fixed 146-symbol sync vector, transcribed verbatim from OZ2M's PI4Ino
// reference (la1k-cw-beacon/other/BeaconPI4CW.ino).
const uint8_t PI4Vector[146] = {
  0,0,1,0,0,1,1,1,1,0,1,0,1,0,1,0,0,1,0,0,0,1,0,0,0,1,1,0,0,1,
  1,1,1,0,0,1,1,1,1,1,0,0,1,1,0,1,1,1,1,0,1,0,1,1,0,1,1,0,1,0,
  0,0,0,0,1,1,1,1,1,0,1,0,1,0,0,0,0,0,1,1,1,1,1,0,1,0,0,1,0,0,
  1,0,1,0,0,0,0,1,0,0,1,1,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,1,1,1,
  0,1,1,1,0,1,1,0,1,0,1,0,1,0,0,0,0,1,1,1,0,0,0,0,1,1,
};

int parity32(uint32_t value) {
  int even = 0;
  for (int bit = 0; bit < 32; bit++)
    if ((value >> bit) & 1) even = 1 - even;
  return even;
}

} // namespace

namespace PI4 {

bool encode(const char *message, uint8_t tones[SYMBOL_COUNT]) {
  if (!message) return false;

  char msg[MAX_MESSAGE_LEN];
  for (int i = 0; i < MAX_MESSAGE_LEN; i++) {
    char c = message[i] ? toupper((unsigned char)message[i]) : ' ';
    if (!strchr(PI4Chars, c)) return false; // reject chars outside PI4's alphabet
    msg[i] = c;
  }

  // Source encoding: base-38 Horner over the 8 characters.
  uint64_t sourceEnc = 0;
  for (int i = 0; i < MAX_MESSAGE_LEN; i++)
    sourceEnc = sourceEnc * 38 + (uint64_t)(strchr(PI4Chars, msg[i]) - PI4Chars);

  // Rate-1/2 convolutional encoding.
  const uint32_t poly1 = 0xF2D05351;
  const uint32_t poly2 = 0xE4613C47;
  uint32_t n = 0;
  uint8_t convEnc[SYMBOL_COUNT];
  int idx = 0;
  for (int j = 0; j < SYMBOL_COUNT / 2; j++) {
    n <<= 1;
    if ((sourceEnc & 0x20000000000ULL) != 0) n |= 1;
    sourceEnc <<= 1;
    convEnc[idx++] = parity32(n & poly1);
    convEnc[idx++] = parity32(n & poly2);
  }

  // Bit-reversal interleaving over a 256-entry permutation, keeping only
  // the first SYMBOL_COUNT of both domain and range.
  int p = 0;
  uint8_t interleaved[SYMBOL_COUNT] = {0};
  for (int i = 0; i <= 255; i++) {
    int r = 0;
    for (int bitNo = 0; bitNo <= 7; bitNo++)
      if ((i >> bitNo) & 1) r |= 1 << (7 - bitNo);
    if (p < SYMBOL_COUNT && r < SYMBOL_COUNT) interleaved[r] = convEnc[p++];
  }

  // Merge with the fixed sync vector to select one of 4 tones per symbol.
  for (int i = 0; i < SYMBOL_COUNT; i++)
    tones[i] = PI4Vector[i] + 2 * interleaved[i];

  return true;
}

} // namespace PI4
