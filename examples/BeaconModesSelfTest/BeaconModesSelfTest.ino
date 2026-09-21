// Self-test for the BeaconModes library.
//
// Verifies Q65Encoder against three known-good message -> tone-sequence
// pairs (pulled from a real, on-air-tested Q65 beacon's previously
// hand-computed tables) and exercises PI4/JT4 encoding so a compile+upload
// also confirms JTEncode links correctly.
#include <BeaconModes.h>

struct Q65Test {
  const char *msg;
  uint8_t expected[Q65::SYMBOL_COUNT];
};

Q65Test q65tests[] = {
  {"DE SR6LEG JO81", {0, 1, 1, 1, 1, 2, 40, 37, 0, 7, 55, 0, 0, 50, 0, 6, 5, 19, 51, 51, 51, 0, 0, 52, 39, 0, 0, 33, 18, 45, 13, 63, 0, 63, 0, 57, 57, 0, 47, 54, 50, 40, 57, 57, 62, 0, 6, 35, 35, 0, 62, 48, 25, 33, 0, 33, 33, 37, 37, 0, 30, 0, 21, 2, 38, 0, 9, 64, 0, 62, 61, 61, 49, 0, 49, 0, 46, 28, 19, 39, 17, 4, 4, 57, 0}},
  {"DE SR6LB JO70",  {0, 1, 1, 1, 1, 2, 40, 37, 0, 7, 44, 0, 0, 50, 0, 6, 2, 35, 48, 48, 48, 0, 0, 47, 45, 0, 0, 14, 61, 34, 2, 52, 0, 52, 0, 54, 54, 0, 18, 38, 34, 62, 35, 35, 40, 0, 32, 57, 57, 0, 13, 47, 26, 5, 0, 5, 5, 6, 6, 0, 61, 0, 41, 62, 26, 0, 3, 54, 0, 43, 28, 28, 24, 0, 24, 0, 11, 61, 54, 61, 24, 4, 4, 57, 0}},
  {"DE SR3LES JO81", {0, 1, 1, 1, 1, 2, 40, 35, 0, 20, 36, 0, 0, 10, 0, 6, 5, 19, 10, 10, 10, 0, 0, 9, 26, 0, 0, 32, 23, 48, 16, 62, 0, 62, 0, 47, 47, 0, 57, 9, 31, 9, 48, 48, 43, 0, 50, 23, 23, 0, 45, 63, 10, 12, 0, 12, 12, 16, 16, 0, 43, 0, 57, 46, 16, 0, 61, 42, 0, 44, 43, 43, 17, 0, 17, 0, 14, 60, 39, 14, 47, 62, 62, 57, 0}},
};

void runTests() {
  Serial.println("=== BeaconModes self-test ===");

  bool allOk = true;
  uint8_t tones[BeaconModes::MAX_SYMBOLS];
  for (auto &t : q65tests) {
    uint16_t n = BeaconModes::encode(BeaconMode::Q65, t.msg, tones);
    bool ok = (n == Q65::SYMBOL_COUNT);
    for (uint16_t i = 0; ok && i < Q65::SYMBOL_COUNT; i++)
      if (tones[i] != t.expected[i]) ok = false;
    Serial.print(t.msg);
    Serial.println(ok ? " : MATCH" : " : MISMATCH");
    if (!ok) allOk = false;
  }
  Serial.println(allOk ? "Q65: ALL TESTS PASSED" : "Q65: SOME TESTS FAILED");

  uint16_t nPi4 = BeaconModes::encode(BeaconMode::PI4, "SR3LES", tones);
  Serial.print("PI4 encode(\"SR3LES\") symbols: ");
  Serial.println(nPi4);
  Serial.print("PI4 first 16 tones: ");
  for (int i = 0; i < 16 && i < nPi4; i++) { Serial.print(tones[i]); Serial.print(' '); }
  Serial.println();

  uint16_t nJt4 = BeaconModes::encode(BeaconMode::JT4, "SR3LES JO81", tones);
  Serial.print("JT4 encode(\"SR3LES JO81\") symbols: ");
  Serial.println(nJt4);
  Serial.print("JT4 first 16 tones: ");
  for (int i = 0; i < 16 && i < nJt4; i++) { Serial.print(tones[i]); Serial.print(' '); }
  Serial.println();

  // Submode parameter check: Q65-60D (this beacon's previous hardcoded
  // config) and JT4G (previous JT4_TONE_SPACING) must reproduce the exact
  // constants that used to be hardcoded, and a few other known submodes
  // should match the public spacing tables.
  Serial.println("--- submode parameter check ---");
  Serial.print("Q65-60D spacing (want 13.333): ");
  Serial.println(Q65::toneSpacingHz(Q65::Duration::T60, Q65::Bandwidth::D), 4);
  Serial.print("Q65-60D period ms (want 600): ");
  Serial.println(Q65::symbolPeriodMs(Q65::Duration::T60), 1);
  Serial.print("Q65-15A spacing (want 6.667): ");
  Serial.println(Q65::toneSpacingHz(Q65::Duration::T15, Q65::Bandwidth::A), 4);
  Serial.print("Q65-15A period ms (want 150): ");
  Serial.println(Q65::symbolPeriodMs(Q65::Duration::T15), 1);
  Serial.print("Q65-300E spacing (want 4.630): ");
  Serial.println(Q65::toneSpacingHz(Q65::Duration::T300, Q65::Bandwidth::E), 2);
  Serial.print("Q65-300E period ms (want 3456): ");
  Serial.println(Q65::symbolPeriodMs(Q65::Duration::T300), 1);

  Serial.print("JT4G spacing (want 315.0): ");
  Serial.println(JT4::toneSpacingHz(JT4::Submode::G), 3);
  Serial.print("JT4A spacing (want 4.375): ");
  Serial.println(JT4::toneSpacingHz(JT4::Submode::A), 3);
  Serial.print("JT4D spacing (want 39.375, the non-doubling step): ");
  Serial.println(JT4::toneSpacingHz(JT4::Submode::D), 3);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
}

void loop() {
  runTests();
  delay(3000);
}
