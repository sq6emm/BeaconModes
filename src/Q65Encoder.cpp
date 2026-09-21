#include "Q65Encoder.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

using namespace std;

namespace {

// ---------------- GF(64) arithmetic (x^6+x+1) ----------------
const int gf64log[64] = {
  -1,   0,   1,   6,   2,  12,   7,  26,   3,  32,
  13,  35,   8,  48,  27,  18,   4,  24,  33,  16,
  14,  52,  36,  54,   9,  45,  49,  38,  28,  41,
  19,  56,   5,  62,  25,  11,  34,  31,  17,  47,
  15,  23,  53,  51,  37,  44,  55,  40,  10,  61,
  46,  30,  50,  22,  39,  43,  29,  60,  42,  21,
  20,  59,  57,  58
};
const int gf64antilog[63] = {
  1,   2,   4,   8,  16,  32,   3,   6,  12,  24,
  48,  35,   5,  10,  20,  40,  19,  38,  15,  30,
  60,  59,  53,  41,  17,  34,   7,  14,  28,  56,
  51,  37,   9,  18,  36,  11,  22,  44,  27,  54,
  47,  29,  58,  55,  45,  25,  50,  39,  13,  26,
  52,  43,  21,  42,  23,  46,  31,  62,  63,  61,
  57,  49,  33
};
inline int gf64_mult(int a, int b) {
  if (a == 0 || b == 0) return 0;
  if (a == 1) return b;
  if (b == 1) return a;
  int j = (gf64log[a] + gf64log[b]) % 63;
  return gf64antilog[j];
}
inline int gf64_add(int a, int b) { return (a ^ b) & 63; }

// Transcribed verbatim from q65_encoding_modules.f90's `generator(15,50)`
// DATA block (Fortran is column-major, so each printed line of 15 numbers
// IS one column: genT[k][i] == Fortran generator(i+1, k+1)).
const int genT[50][15] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  { 0,20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  { 0,20, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  { 0,20, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  { 0,20, 0, 1, 1, 0, 0, 0,10, 0, 0, 0, 0, 1, 0},
  { 0,20, 0, 1, 1, 0, 0, 0,10, 0, 0, 0,44, 1, 0},
  { 0,20, 0, 1, 1, 0, 0, 0,10, 1, 0, 0,44, 1, 0},
  { 0,20, 0, 1, 1, 0, 0, 0,10, 1, 0, 0,44, 1,14},
  { 0,20, 0, 1, 1, 0, 0, 0,10, 1,31, 0,44, 1,14},
  { 0,20, 0, 1, 1,33, 0, 0,10, 1,31, 0,44, 1,14},
  {56,20, 0, 1, 1,33, 0, 0,10, 1,31, 0,44, 1,14},
  {56,20, 0, 1, 1,33, 0, 1,10, 1,31, 0,44, 1,14},
  {56, 1, 0, 1, 1,33, 0, 1,10, 1,31, 0,44, 1,14},
  {56, 1, 0, 1, 1,33, 0, 1,10, 1,31,36,44, 1,14},
  {56, 1, 0, 1, 1,33, 0, 1,43, 1,31,36,44, 1,14},
  {56, 1, 0, 1, 1,33, 0, 1,43,17,31,36,44, 1,14},
  {56, 1, 0, 1, 1,33, 0, 1,43,17,31,36,36, 1,14},
  {56, 1, 0, 1, 1,33,53, 1,43,17,31,36,36, 1,14},
  {56, 1, 0,35, 1,33,53, 1,43,17,31,36,36, 1,14},
  {56, 1, 0,35, 1,33,53, 1,43,17,30,36,36, 1,14},
  {56, 1, 0,35, 1,33,53,52,43,17,30,36,36, 1,14},
  {56, 1, 0,35, 1,32,53,52,43,17,30,36,36, 1,14},
  {56, 1,60,35, 1,32,53,52,43,17,30,36,36, 1,14},
  {56, 1,60,35, 1,32,53,52,43,17,30,36,36,49,14},
  {56, 1,60,35, 1,32,53,52,43,17,30,36,37,49,14},
  {56, 1,60,35,54,32,53,52,43,17,30,36,37,49,14},
  {56, 1,60,35,54,32,53,52, 1,17,30,36,37,49,14},
  { 1, 1,60,35,54,32,53,52, 1,17,30,36,37,49,14},
  { 1, 0,60,35,54,32,53,52, 1,17,30,36,37,49,14},
  { 1, 0,60,35,54,32,53,52, 1,17,30,37,37,49,14},
  { 1, 0,61,35,54,32,53,52, 1,17,30,37,37,49,14},
  { 1, 0,61,35,54,32,53,52, 1,48,30,37,37,49,14},
  { 1, 0,61,35,54,32,53,52, 1,48,30,37,37,49,15},
  { 1, 0,61,35,54, 0,53,52, 1,48,30,37,37,49,15},
  { 1, 0,61,35,54, 0,52,52, 1,48,30,37,37,49,15},
  { 1, 0,61,35,54, 0,52,52, 1,48,30,37,37, 0,15},
  { 1, 0,61,35,54, 0,52,34, 1,48,30,37,37, 0,15},
  { 1, 0,61,35,54, 0,52,34, 1,48,30,37, 0, 0,15},
  { 1, 0,61,35,54, 0,52,34, 1,48,30,20, 0, 0,15},
  { 1, 0, 0,35,54, 0,52,34, 1,48,30,20, 0, 0,15},
  { 1, 0, 0,35,54, 0,52,34, 1, 0,30,20, 0, 0,15},
  { 0, 0, 0,35,54, 0,52,34, 1, 0,30,20, 0, 0,15},
  { 0, 0, 0,35,54, 0,52,34, 1, 0,38,20, 0, 0,15},
  { 0, 0, 0,35, 0, 0,52,34, 1, 0,38,20, 0, 0,15},
  { 0, 0, 0,35, 0, 0,52, 0, 1, 0,38,20, 0, 0,15},
  { 0, 0, 0,35, 0, 0,52, 0, 1, 0,38,20, 0, 0, 0},
  { 0, 0, 0,35, 0, 0,52, 0, 0, 0,38,20, 0, 0, 0},
  { 0, 0, 0,35, 0, 0,52, 0, 0, 0,38, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0,52, 0, 0, 0,38, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,38, 0, 0, 0, 0},
};

// codeword[0..14]=message (systematic), codeword[15..64] = 50 parity symbols
void q65_encode_gf(const int message[15], int codeword[65]) {
  for (int i = 0; i < 65; i++) codeword[i] = 0;
  for (int i = 0; i < 15; i++) codeword[i] = message[i];
  for (int k = 0; k < 50; k++) {
    int acc = 0;
    for (int i = 0; i < 15; i++) acc = gf64_add(acc, gf64_mult(message[i], genT[k][i]));
    codeword[15 + k] = acc;
  }
}

// ---------------- 12-bit CRC (ported from get_q65crc12) ----------------
// bits[0..76] = the 77-bit packed message, bits[77] = 0 padding bit,
// bits[78..89] filled in with the CRC on return (90 bits total).
void get_q65crc12(int bits[90]) {
  static const int p[13] = {1,1,0,0,0,0,0,0,0,1,1,1,1};
  int mc[90];
  // Reverse bit order within each successive 6-bit chunk ("for consistency
  // with Nico's calculation") before running the LFSR.
  for (int i = 0; i < 15; i++)
    for (int j = 0; j < 6; j++)
      mc[i*6+j] = bits[i*6 + (5-j)];

  int r[13];
  for (int k = 0; k < 13; k++) r[k] = mc[k];
  for (int i = 0; i < 78; i++) {
    r[12] = mc[i+12];
    if (r[0] == 1) for (int k = 0; k < 13; k++) r[k] ^= p[k];
    int first = r[0];
    for (int k = 0; k < 12; k++) r[k] = r[k+1];
    r[12] = first;
  }
  bits[78]=r[5]; bits[79]=r[4]; bits[80]=r[3]; bits[81]=r[2]; bits[82]=r[1]; bits[83]=r[0];
  bits[84]=r[11]; bits[85]=r[10]; bits[86]=r[9]; bits[87]=r[8]; bits[88]=r[7]; bits[89]=r[6];
}

// ---------------- pack77 message packing (subset ported from packjt77.f90) ----------------
string upper(string s) { for (auto &c : s) c = toupper((unsigned char)c); return s; }
bool is_digit_c(char c){ return c>='0'&&c<='9'; }
bool is_letter_c(char c){ return c>='A'&&c<='Z'; }

// Port of chkcall: returns true + 6-char base call if w looks like a valid
// standard or compound callsign.
bool chkcall(string w, string &bc) {
  bc = w.substr(0, min<size_t>(6, w.size()));
  bc.resize(6, ' ');
  int n1 = (int)w.size();
  if (n1 > 11) return false;
  if (w.find('.') != string::npos) return false;
  if (w.find('+') != string::npos) return false;
  if (w.find('-') != string::npos) return false;
  if (w.find('?') != string::npos) return false;
  size_t slash = w.find('/');
  if (n1 > 6 && slash == string::npos) return false;
  if (slash != string::npos) {
    int i0 = (int)slash + 1; // 1-indexed position of '/'
    int before = i0 - 1, after = n1 - i0;
    if (max(before, after) > 6) return false;
    if (i0 >= 2 && i0 <= n1 - 1) {
      if (before <= after) bc = w.substr(i0) + "   ";
      else bc = w.substr(0, i0 - 1) + "   ";
      bc.resize(6, ' ');
    }
  }
  string bct = bc; while (!bct.empty() && bct.back()==' ') bct.pop_back();
  int nbc = (int)bct.size();
  if (nbc > 6) return false;
  if (!is_letter_c(bc[0]) && !is_letter_c(bc[1])) return false;
  if (bc[0]=='Q' && bc.substr(0,5)!="QU1RK") return false;
  int i1 = 0;
  if (is_digit_c(bc[1])) i1 = 2;
  if (is_digit_c(bc[2])) i1 = 3;
  if (i1 == 0) return false;
  if (i1 == nbc) return false;
  int n = 0;
  for (int i = i1; i < nbc; i++) { // 0-indexed i1..nbc-1 == Fortran i1+1..nbc
    if (!is_letter_c(bc[i])) return false;
    n++;
  }
  if (n >= 1 && n <= 3) { bc = bct; return true; }
  return false;
}

// Port of pack28's "special token" and "standard callsign" paths only (the
// <...>/nonstandard-hash paths aren't reachable by CALL/GRID-style beacon
// messages and are intentionally omitted).
bool pack28(string c13, uint32_t &n28) {
  string s = c13; s.resize(13, ' ');
  if (s.substr(0,3) == "DE ") { n28 = 0; return true; }
  if (s.substr(0,4) == "QRZ ") { n28 = 1; return true; }
  if (s.substr(0,3) == "CQ ") { n28 = 2; return true; }

  string trimmed = c13; while(!trimmed.empty() && trimmed.back()==' ') trimmed.pop_back();
  int n = (int)trimmed.size();
  int iarea = -1;
  for (int i = n; i >= 2; i--) {
    if (is_digit_c(trimmed[i-1])) { iarea = i; break; }
  }
  if (iarea < 0) return false; // no digit found -> would need hash (unsupported)
  int npdig = 0, nplet = 0;
  for (int i = 1; i <= iarea - 1; i++) {
    if (is_digit_c(trimmed[i-1])) npdig++;
    if (is_letter_c(trimmed[i-1])) nplet++;
  }
  int nslet = 0;
  for (int i = iarea + 1; i <= n; i++) if (is_letter_c(trimmed[i-1])) nslet++;
  if (iarea < 2 || iarea > 3 || nplet == 0 || npdig >= iarea - 1 || nslet > 3)
    return false; // nonstandard -> would need 22-bit hash (unsupported)

  string callsign;
  if (iarea == 2) callsign = " " + trimmed.substr(0, 5);
  else callsign = trimmed.substr(0, 6);
  callsign.resize(6, ' ');

  static const string a1 = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static const string a2 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static const string a3 = "0123456789";
  static const string a4 = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i1v = (int)a1.find(callsign[0]);
  int i2v = (int)a2.find(callsign[1]);
  int i3v = (int)a3.find(callsign[2]);
  int i4v = (int)a4.find(callsign[3]);
  int i5v = (int)a4.find(callsign[4]);
  int i6v = (int)a4.find(callsign[5]);
  uint32_t val = 36u*10*27*27*27*i1v + 10u*27*27*27*i2v + 27u*27*27*i3v + 27u*27*i4v + 27u*i5v + i6v;
  val += 2063592u + 4194304u; // NTOKENS + MAX22
  n28 = val & ((1u<<28)-1);
  return true;
}

bool is_grid4(const string &g) {
  if (g.size() != 4) return false;
  return g[0]>='A'&&g[0]<='R' && g[1]>='A'&&g[1]<='R' &&
         g[2]>='0'&&g[2]<='9' && g[3]>='0'&&g[3]<='9';
}

void put_bits_msb(vector<int> &c77, int pos, uint32_t value, int nbits) {
  for (int i = 0; i < nbits; i++) c77[pos+i] = (value >> (nbits-1-i)) & 1;
}

// Port of pack77_1 (Type 1/2 standard message). Returns false if this
// message doesn't fit the pattern.
bool pack77_1(const vector<string> &w, vector<int> &c77) {
  int nwords = (int)w.size();
  if (nwords < 2 || nwords > 4) return false;
  string bcall_1, bcall_2;
  bool ok1 = chkcall(w[0], bcall_1);
  bool ok2 = chkcall(w[1], bcall_2);
  string w1p = w[0] + " ";
  if (w1p.substr(0,3)=="DE " || w1p.substr(0,3)=="CQ " || w1p.substr(0,4)=="QRZ ") ok1 = true;
  if (!ok1 || !ok2) return false;

  int nlast = nwords - 1; // 0-indexed last word
  const string &last = w[nlast];
  char c1 = last.empty() ? ' ' : last[0];
  string c2 = last.substr(0, min<size_t>(2,last.size()));
  bool grid = is_grid4(last.substr(0, min<size_t>(4,last.size())));
  if (!grid && c1!='+' && c1!='-' && c2!="R+" && c2!="R-" &&
      last!="RRR" && last!="RR73" && last!="73") return false;

  int i3;
  bool w3isR = (nwords==4 && w[2]=="R");
  if (nwords==2 || nwords==3 || (nwords==4 && w3isR)) {
    i3 = 1;
  } else return false;

  uint32_t n28a; if (!pack28(bcall_1, n28a)) return false;
  uint32_t n28b; if (!pack28(bcall_2, n28b)) return false;

  int ipa=0, ipb=0; // no /P or /R suffix support needed for this beacon
  int ir = 0;
  uint32_t igrid4;
  if (grid) {
    if (w3isR) ir = 1;
    int j1=(last[0]-'A')*18*10*10, j2=(last[1]-'A')*10*10, j3=(last[2]-'0')*10, j4=(last[3]-'0');
    igrid4 = j1+j2+j3+j4;
  } else {
    igrid4 = 32400; // MAXGRID4 (report path unused by this beacon)
  }
  if (nwords == 2) { ir = 0; igrid4 = 32400 + 1; }

  c77.assign(77, 0);
  int pos = 0;
  put_bits_msb(c77, pos, n28a, 28); pos+=28;
  put_bits_msb(c77, pos, ipa, 1); pos+=1;
  put_bits_msb(c77, pos, n28b, 28); pos+=28;
  put_bits_msb(c77, pos, ipb, 1); pos+=1;
  put_bits_msb(c77, pos, ir, 1); pos+=1;
  put_bits_msb(c77, pos, igrid4, 15); pos+=15;
  put_bits_msb(c77, pos, i3, 3); pos+=3;
  return true;
}

// Port of packtext77: base-42 free text packing of up to 13 characters,
// using a 9-byte (72-bit) big-endian byte array as the bignum accumulator
// (mirrors WSJT-X's own mp_short_mult/mp_short_add byte-array approach;
// avoids needing a 128-bit integer type, which this toolchain lacks).
void packtext77(string msg13, vector<int> &c71) {
  static const string alpha = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ+-./?";
  msg13.resize(13, ' ');
  string trimmedRight = msg13; while(!trimmedRight.empty() && trimmedRight.back()==' ') trimmedRight.pop_back();
  string rj = string(13 - trimmedRight.size(), ' ') + trimmedRight;

  uint8_t qa[9] = {0,0,0,0,0,0,0,0,0};
  for (int i = 0; i < 13; i++) {
    int j = (int)alpha.find(rj[i]);
    if (j < 0) j = 0;
    uint32_t carry = (uint32_t)j;
    for (int k = 8; k >= 0; k--) {
      uint32_t v = (uint32_t)qa[k] * 42 + carry;
      qa[k] = v & 0xFF;
      carry = v >> 8;
    }
  }
  c71.assign(71, 0);
  for (int i = 0; i < 71; i++) {
    int byteIdx = 8 - i / 8;
    int bit = (qa[byteIdx] >> (i % 8)) & 1;
    c71[70 - i] = bit;
  }
}

vector<string> split_words(const string &msg0) {
  string msg = upper(msg0);
  vector<string> words;
  string cur;
  for (char c : msg) {
    if (c == ' ') { if (!cur.empty()) { words.push_back(cur); cur.clear(); } }
    else cur += c;
  }
  if (!cur.empty()) words.push_back(cur);
  return words;
}

// Simplified pack77 dispatcher: try Type-1 standard message, else free text.
void pack77(const string &msg0, vector<int> &c77) {
  vector<string> w = split_words(msg0);
  if (pack77_1(w, c77)) return;

  string msg = upper(msg0);
  string collapsed;
  bool lastSpace = true;
  for (char c : msg) {
    if (c == ' ') { if (!lastSpace) collapsed += ' '; lastSpace = true; }
    else { collapsed += c; lastSpace = false; }
  }
  while (!collapsed.empty() && collapsed.back()==' ') collapsed.pop_back();
  string text13 = collapsed.substr(0, min<size_t>(13, collapsed.size()));
  vector<int> c71;
  packtext77(text13, c71);
  c77.assign(77, 0);
  for (int i = 0; i < 71; i++) c77[i] = c71[i];
  // n3=0, i3=0 -> trailing 6 bits already zero
}

const int syncPos[22] = {1,9,12,13,15,22,23,26,27,33,35,38,46,50,55,60,62,66,69,74,76,85}; // 1-indexed

} // namespace

namespace Q65 {

bool encode(const char *message, uint8_t tones[SYMBOL_COUNT]) {
  if (!message) return false;
  vector<int> c77;
  pack77(string(message), c77);

  int bits[90];
  for (int i = 0; i < 77; i++) bits[i] = c77[i];
  bits[77] = 0; // pad to 78 bits
  for (int i = 78; i < 90; i++) bits[i] = 0;
  get_q65crc12(bits);

  int msgSyms[15];
  for (int i = 0; i < 15; i++) {
    int v = 0;
    for (int j = 0; j < 6; j++) v = (v<<1) | bits[i*6+j];
    msgSyms[i] = v;
  }

  int codeword[65];
  q65_encode_gf(msgSyms, codeword);

  int shortcw[63];
  for (int i = 0; i < 13; i++) shortcw[i] = codeword[i];
  for (int i = 0; i < 50; i++) shortcw[13+i] = codeword[15+i];

  int j = 0, k = 0;
  for (int i = 1; i <= 85; i++) {
    if (j < 22 && i == syncPos[j]) { tones[i-1] = 0; j++; }
    else { tones[i-1] = (uint8_t)(shortcw[k] + 1); k++; }
  }
  return true;
}

// Ported directly from WSJT-X's lib/q65params.f90.
float symbolPeriodMs(Duration duration) {
  static const uint16_t nsps[5] = {1800, 3600, 7200, 16000, 41472}; // T=15,30,60,120,300s
  return nsps[(int)duration] / 12.0f; // = 1000 * nsps/12000 (12000 Hz reference sample rate)
}

float toneSpacingHz(Duration duration, Bandwidth bandwidth) {
  static const uint16_t nsps[5] = {1800, 3600, 7200, 16000, 41472};
  float baud = 12000.0f / nsps[(int)duration];
  return baud * (float)(1 << (int)bandwidth);
}

} // namespace Q65
