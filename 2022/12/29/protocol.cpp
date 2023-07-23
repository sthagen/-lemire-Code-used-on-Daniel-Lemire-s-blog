
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <unordered_set>
#include <string_view>
#include <regex>
#include "gperf.c"

uint64_t nano() {
  return std::chrono::duration_cast<::std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

static inline uint64_t string_to_uint64(std::string_view view) {
  uint64_t val;
  std::memcpy(&val, view.data(), sizeof(uint64_t));
  return val;
}

static inline uint64_t string_to_uint64(const uint8_t * data) {
  uint64_t val;
  std::memcpy(&val, data, sizeof(uint64_t));
  return val;
}

uint32_t string_to_uint32(const char *data) {
  uint32_t val;
  std::memcpy(&val, data, sizeof(uint32_t));
  return val;
}

std::string_view table_craftedhash_is_special[] = {"http", "", "https", 
  "ws", "ftp", "wss", "file", ""};

bool craftedhash_is_special(std::string_view input) {
  if(input.empty()) { return false; }
  int hash_value = (2*input.size() + (unsigned)(input[0])) & 7;
  const std::string_view target = table_craftedhash_is_special[hash_value];
  return (target[0] == input[0]) && (target.substr(1) == input.substr(1));
}

const uint8_t mulhi_table[64] = {
    'w', 's', 0,   0, 0, 0, 0, 0, 'f', 't', 'p', 0,   0,   0, 0, 0,
    'w', 's', 's', 0, 0, 0, 0, 0, 'h', 't', 't', 'p', 's', 0, 0, 0,
    0,   0,   0,   0, 0, 0, 0, 0, 'f', 'i', 'l', 'e', 0,   0, 0, 0,
    0,   0,   0,   0, 0, 0, 0, 0, 'h', 't', 't', 'p', 0,   0, 0, 0};

bool mulhi_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  __uint128_t magic = 0x123ULL << 37;
  return string_to_uint64(
             mulhi_table + (((inputu * magic) >> 64) & 0x38)) ==
         inputu;
}

__attribute__((noinline))
bool no_inline_mulhi_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  __uint128_t magic = 0x123ULL << 37;
  return string_to_uint64(
             mulhi_table + (((inputu * magic) >> 64) & 0x38)) ==
         inputu;
}

static const uint8_t shiftxor_table[128] = {
    'w', 's', 0,   0,   0,   0, 0, 0, 0,   0,   0,   0, 0, 0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0, 0, 0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0, 0, 0, 0, 0,
    'f', 'i', 'l', 'e', 0,   0, 0, 0, 0,   0,   0,   0, 0, 0, 0, 0,
    'f', 't', 'p', 0,   0,   0, 0, 0, 'w', 's', 's', 0, 0, 0, 0, 0,
    'h', 't', 't', 'p', 0,   0, 0, 0, 0,   0,   0,   0, 0, 0, 0, 0,
    'h', 't', 't', 'p', 's', 0, 0, 0, 0,   0,   0,   0, 0, 0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0, 0, 0, 0, 0};

bool shiftxor_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);

  return string_to_uint64(
	     shiftxor_table + (((inputu >> 28) ^ (inputu >> 14)) & 0x78)) ==
         inputu;
}

__attribute__((noinline))
bool no_inline_shiftxor_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);

  return string_to_uint64(
	     shiftxor_table + (((inputu >> 28) ^ (inputu >> 14)) & 0x78)) ==
         inputu;
}

#define DFA_STATE_MATCH   0 /* For easier comparisons. */
#define DFA_STATE_FAIL    1
#define DFA_STATE_INIT    2
#define DFA_STATE_HTTP_1  3
#define DFA_STATE_HTTP_2  4
#define DFA_STATE_HTTP_3  5
#define DFA_STATE_S       6
#define DFA_STATE_HTTP_4  DFA_STATE_S
#define DFA_STATE_NUL     7
#define DFA_STATE_HTTPS_1 DFA_STATE_NUL
#define DFA_STATE_F_1     8
#define DFA_STATE_FILE_1  9
#define DFA_STATE_FILE_2  10
#define DFA_STATE_FILE_3  DFA_STATE_NUL
#define DFA_STATE_FTP_1   11
#define DFA_STATE_FTP_2   DFA_STATE_NUL
#define DFA_STATE_WS_1    12
#define DFA_STATE_WS_2    DFA_STATE_S
#define DFA_STATE_WSS_1   DFA_STATE_NUL

#define DFA_STATES_COUNT  13

static uint8_t dfa_states[DFA_STATES_COUNT][256];

void init_dfa_states() {
  memset(dfa_states[DFA_STATE_INIT], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_INIT][(uint8_t)'h'] = DFA_STATE_HTTP_1;
  dfa_states[DFA_STATE_INIT][(uint8_t)'f'] = DFA_STATE_F_1;
  dfa_states[DFA_STATE_INIT][(uint8_t)'w'] = DFA_STATE_WS_1;

  memset(dfa_states[DFA_STATE_HTTP_1], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_HTTP_1][(uint8_t)'t'] = DFA_STATE_HTTP_2;

  memset(dfa_states[DFA_STATE_HTTP_2], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_HTTP_2][(uint8_t)'t'] = DFA_STATE_HTTP_3;

  memset(dfa_states[DFA_STATE_HTTP_3], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_HTTP_3][(uint8_t)'p'] = DFA_STATE_HTTP_4;

  /* HTTP_4, WS_2 */
  memset(dfa_states[DFA_STATE_S], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_S][(uint8_t)'\0'] = DFA_STATE_MATCH;
  dfa_states[DFA_STATE_S][(uint8_t)'s'] = DFA_STATE_NUL;

  /* HTTPS_1, FILE_3, FTP_2, WSS_1 */
  memset(dfa_states[DFA_STATE_NUL], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_NUL][(uint8_t)'\0'] = DFA_STATE_MATCH;

  memset(dfa_states[DFA_STATE_F_1], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_F_1][(uint8_t)'i'] = DFA_STATE_FILE_1;
  dfa_states[DFA_STATE_F_1][(uint8_t)'t'] = DFA_STATE_FTP_1;

  memset(dfa_states[DFA_STATE_FILE_1], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_FILE_1][(uint8_t)'l'] = DFA_STATE_FILE_2;

  memset(dfa_states[DFA_STATE_FILE_2], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_FILE_2][(uint8_t)'e'] = DFA_STATE_FILE_3;

  memset(dfa_states[DFA_STATE_FTP_1], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_FTP_1][(uint8_t)'p'] = DFA_STATE_FTP_2;

  memset(dfa_states[DFA_STATE_WS_1], DFA_STATE_FAIL, sizeof(*dfa_states));
  dfa_states[DFA_STATE_WS_1][(uint8_t)'s'] = DFA_STATE_WS_2;

  memset(dfa_states[DFA_STATE_MATCH], DFA_STATE_MATCH, sizeof(*dfa_states));
  memset(dfa_states[DFA_STATE_FAIL], DFA_STATE_FAIL, sizeof(*dfa_states));
}

static void *dfa_states2[DFA_STATES_COUNT][256];

static void set_transition(uint64_t state, char c, uint64_t val) {
  dfa_states2[state][(uint8_t)c] = dfa_states2[val];
}

static void set_default_transition(uint64_t state, uint64_t val) {
  for (size_t i = 0; i < 256; i++) {
    dfa_states2[state][i] = dfa_states2[val];
  }
}

void init_dfa_states2() {
  set_default_transition(DFA_STATE_INIT, DFA_STATE_FAIL);
  set_transition(DFA_STATE_INIT, 'h', DFA_STATE_HTTP_1);
  set_transition(DFA_STATE_INIT, 'f', DFA_STATE_F_1);
  set_transition(DFA_STATE_INIT, 'w', DFA_STATE_WS_1);

  set_default_transition(DFA_STATE_HTTP_1, DFA_STATE_FAIL);
  set_transition(DFA_STATE_HTTP_1, 't', DFA_STATE_HTTP_2);

  set_default_transition(DFA_STATE_HTTP_2, DFA_STATE_FAIL);
  set_transition(DFA_STATE_HTTP_2, 't', DFA_STATE_HTTP_3);

  set_default_transition(DFA_STATE_HTTP_3, DFA_STATE_FAIL);
  set_transition(DFA_STATE_HTTP_3, 'p', DFA_STATE_HTTP_4);

  /* HTTP_4, WS_2 */
  set_default_transition(DFA_STATE_S, DFA_STATE_FAIL);
  set_transition(DFA_STATE_S, '\0', DFA_STATE_MATCH);
  set_transition(DFA_STATE_S, 's', DFA_STATE_NUL);

  /* HTTPS_1, FILE_3, FTP_2, WSS_1 */
  set_default_transition(DFA_STATE_NUL, DFA_STATE_FAIL);
  set_transition(DFA_STATE_NUL, '\0', DFA_STATE_MATCH);

  set_default_transition(DFA_STATE_F_1, DFA_STATE_FAIL);
  set_transition(DFA_STATE_F_1, 'i', DFA_STATE_FILE_1);
  set_transition(DFA_STATE_F_1, 't', DFA_STATE_FTP_1);

  set_default_transition(DFA_STATE_FILE_1, DFA_STATE_FAIL);
  set_transition(DFA_STATE_FILE_1, 'l', DFA_STATE_FILE_2);

  set_default_transition(DFA_STATE_FILE_2, DFA_STATE_FAIL);
  set_transition(DFA_STATE_FILE_2, 'e', DFA_STATE_FILE_3);

  set_default_transition(DFA_STATE_FTP_1, DFA_STATE_FAIL);
  set_transition(DFA_STATE_FTP_1, 'p', DFA_STATE_FTP_2);

  set_default_transition(DFA_STATE_WS_1, DFA_STATE_FAIL);
  set_transition(DFA_STATE_WS_1, 's', DFA_STATE_WS_2);

  set_default_transition(DFA_STATE_MATCH, DFA_STATE_MATCH);
  set_default_transition(DFA_STATE_FAIL, DFA_STATE_FAIL);
}

bool dfa(std::string_view input) {
  uint8_t state = DFA_STATE_INIT;
  const char *str = input.data();

  for (size_t i = 0; i < 6; i++) {
    state = dfa_states[state][(uint8_t)str[i]];

    if (state == DFA_STATE_MATCH || state == DFA_STATE_FAIL)
      break;
  }

  return state == DFA_STATE_MATCH;
}

__attribute__((noinline))
bool no_inline_dfa(std::string_view input) {
  uint8_t state = DFA_STATE_INIT;
  const char *str = input.data();

  for (size_t i = 0; i < 6; i++) {
    state = dfa_states[state][(uint8_t)str[i]];

    if (state == DFA_STATE_MATCH || state == DFA_STATE_FAIL)
      break;
  }

  return state == DFA_STATE_MATCH;
}

bool dfa2(std::string_view input) {
  uint8_t state = DFA_STATE_INIT;
  const char *str = input.data();
  size_t j = 0;

  for (size_t i = 0; i < 6; i++) {
    uint8_t c = (uint8_t)str[j];
    j += c != 0;

    state = dfa_states[state][c];
  }

  return state == DFA_STATE_MATCH;
}

__attribute__((noinline))
bool no_inline_dfa2(std::string_view input) {
  uint8_t state = DFA_STATE_INIT;
  const char *str = input.data();
  size_t j = 0;

  for (size_t i = 0; i < 6; i++) {
    uint8_t c = (uint8_t)str[j];
    j += c != 0;

    state = dfa_states[state][c];
  }

  return state == DFA_STATE_MATCH;
}

bool dfa3(std::string_view input) {
  void *state = (void *)dfa_states2[DFA_STATE_INIT];
  const char *str = input.data();
  size_t j = 0;

  for (size_t i = 0; i < 6; i++) {
    uint8_t c = (uint8_t)str[j];
    j += c != 0;

    state = *((void **)state + c);
  }

  return state == dfa_states2[DFA_STATE_MATCH];
}

__attribute__((noinline))
bool no_inline_dfa3(std::string_view input) {
  void *state = (void *)dfa_states2[DFA_STATE_INIT];
  const char *str = input.data();
  size_t j = 0;

  for (size_t i = 0; i < 6; i++) {
    uint8_t c = (uint8_t)str[j];
    j += c != 0;

    state = *((void **)state + c);
  }

  return state == dfa_states2[DFA_STATE_MATCH];
}

static uint8_t mask_data[6][256];

void init_mask_data() {
  const static char* valid[] = {"https", "http", "ftp", "file", "ws", "wss"};

  for (size_t i = 0; i < 6; i++) {
    for (size_t j = 0; j < 6; j++) {
      if (j <= strlen(valid[i])) {
	mask_data[j][(uint8_t)valid[i][j]] |= 1 << i;
      } else {
	for (size_t k = 0; k < 256; k++) {
	  mask_data[j][(uint8_t)valid[i][k]] |= 1 << i;
	}
      }
    }
  }
}

bool mask(std::string_view input) {
  uint8_t mask = ~0;
  const char *str = input.data();

  for (size_t i = 0; i < 6; i++) {
    mask &= mask_data[i][(uint8_t)str[i]];

    if (!mask)
      break;
  }

  return mask;
}

__attribute__((noinline))
bool no_inline_mask(std::string_view input) {
  uint8_t mask = ~0;
  const char *str = input.data();

  for (size_t i = 0; i < 6; i++) {
    mask &= mask_data[i][(uint8_t)str[i]];

    if (!mask)
      break;
  }

  return mask;
}

bool mask2(std::string_view input) {
  uint8_t mask = ~0;
  size_t j = 0;
  const char *str = input.data();

  for (size_t i = 0; i < 6; i++) {
    uint8_t c = str[j];
    mask &= mask_data[j][c];
    j += c != 0;
  }

  return mask;
}

__attribute__((noinline))
bool no_inline_mask2(std::string_view input) {
  uint8_t mask = ~0;
  size_t j = 0;
  const char *str = input.data();

  for (size_t i = 0; i < 6; i++) {
    uint8_t c = str[j];
    mask &= mask_data[j][c];
    j += c != 0;
  }

  return mask;
}

bool dfa_is_special(std::string_view input) {
  uint8_t state = DFA_STATE_INIT;
  uint64_t inputu = string_to_uint64(input);
  const char *str = (char *)&inputu;

  for (size_t i = 0; i < 6; i++) {
    state = dfa_states[state][(uint8_t)str[i]];
  }

  return state == DFA_STATE_MATCH;
}

__attribute__((noinline))
bool no_inline_dfa_is_special(std::string_view input) {
  uint8_t state = DFA_STATE_INIT;
  uint64_t inputu = string_to_uint64(input);
  const char *str = (char *)&inputu;

  for (size_t i = 0; i < 6; i++) {
    state = dfa_states[state][(uint8_t)str[i]];
  }

  return state == DFA_STATE_MATCH;
}

bool dfa3_is_special(std::string_view input) {
  void *state = (void *)dfa_states2[DFA_STATE_INIT];
  uint64_t inputu = string_to_uint64(input);
  const char *str = (char *)&inputu;

  for (size_t i = 0; i < 6; i++) {
    state = *((void **)state + (uint8_t)str[i]);
  }

  return state == dfa_states2[DFA_STATE_MATCH];
}

__attribute__((noinline))
bool no_inline_dfa3_is_special(std::string_view input) {
  void *state = (void *)dfa_states2[DFA_STATE_INIT];
  uint64_t inputu = string_to_uint64(input);
  const char *str = (char *)&inputu;

  for (size_t i = 0; i < 6; i++) {
    state = *((void **)state + (uint8_t)str[i]);
  }

  return state == dfa_states2[DFA_STATE_MATCH];
}

bool mask3_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  const char *str = (char *)&inputu;
  uint8_t mask1, mask2, mask3;

  mask1 = mask_data[0][(uint8_t)str[0]] & mask_data[1][(uint8_t)str[1]];
  mask2 = mask_data[2][(uint8_t)str[2]] & mask_data[3][(uint8_t)str[3]];
  mask3 = mask_data[4][(uint8_t)str[4]] & mask_data[5][(uint8_t)str[5]];

  return mask1 & mask2 & mask3;
}

__attribute__((noinline))
bool no_inline_mask3_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  const char *str = (char *)&inputu;
  uint8_t mask1, mask2, mask3;

  mask1 = mask_data[0][(uint8_t)str[0]] & mask_data[1][(uint8_t)str[1]];
  mask2 = mask_data[2][(uint8_t)str[2]] & mask_data[3][(uint8_t)str[3]];
  mask3 = mask_data[4][(uint8_t)str[4]] & mask_data[5][(uint8_t)str[5]];

  return mask1 & mask2 & mask3;
}

bool fast_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  if ((inputu & 0xffffffffff) == string_to_uint64("https\0\0\0")) {
    return input.size() == 5;
  }
  if ((inputu & 0xffffffff) == string_to_uint64("http\0\0\0\0")) {
    return input.size() == 4;
  }
  if (uint32_t(inputu) == string_to_uint32("file")) {
    return input.size() == 4;
  }
  if ((inputu & 0xffffff) == string_to_uint32("ftp\0")) {
    return input.size() == 3;
  }
  if ((inputu & 0xffffff) == string_to_uint32("wss\0")) {
    return input.size() == 3;
  }
  if ((inputu & 0xffff) == string_to_uint32("ws\0\0")) {
    return input.size() == 2;
  }
  return false;
}

bool fast2_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  uint64_t https = string_to_uint64("https\0\0\0");
  uint64_t http = string_to_uint64("http\0\0\0\0");
  uint64_t file = string_to_uint64("file\0\0\0\0");
  uint64_t ftp = string_to_uint64("ftp\0\0\0\0\0");
  uint64_t wss = string_to_uint64("wss\0\0\0\0\0");
  uint64_t ws = string_to_uint64("ws\0\0\0\0\0\0");
  if((inputu == https) | (inputu == http)) {
    return true;
  }
  return ((inputu == file) | (inputu == ftp) 
          | (inputu == wss) | (inputu == ws));
}

__attribute__((noinline))
bool no_inline_fast2_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  uint64_t https = string_to_uint64("https\0\0\0");
  uint64_t http = string_to_uint64("http\0\0\0\0");
  uint64_t file = string_to_uint64("file\0\0\0\0");
  uint64_t ftp = string_to_uint64("ftp\0\0\0\0\0");
  uint64_t wss = string_to_uint64("wss\0\0\0\0\0");
  uint64_t ws = string_to_uint64("ws\0\0\0\0\0\0");
  if((inputu == https) | (inputu == http)) {
    return true;
  }
  return ((inputu == file) | (inputu == ftp) 
          | (inputu == wss) | (inputu == ws));
}

bool branchless_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  uint64_t https = string_to_uint64("https\0\0\0");
  uint64_t http = string_to_uint64("http\0\0\0\0");
  uint64_t file = string_to_uint64("file\0\0\0\0");
  uint64_t ftp = string_to_uint64("ftp\0\0\0\0\0");
  uint64_t wss = string_to_uint64("wss\0\0\0\0\0");
  uint64_t ws = string_to_uint64("ws\0\0\0\0\0\0");
  return ((inputu == https) | (inputu == http) 
          | (inputu == file) | (inputu == ftp) 
          | (inputu == wss) | (inputu == ws));
}

__attribute__((noinline))
bool no_inline_branchless_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  uint64_t https = string_to_uint64("https\0\0\0");
  uint64_t http = string_to_uint64("http\0\0\0\0");
  uint64_t file = string_to_uint64("file\0\0\0\0");
  uint64_t ftp = string_to_uint64("ftp\0\0\0\0\0");
  uint64_t wss = string_to_uint64("wss\0\0\0\0\0");
  uint64_t ws = string_to_uint64("ws\0\0\0\0\0\0");
  return ((inputu == https) | (inputu == http) 
          | (inputu == file) | (inputu == ftp) 
          | (inputu == wss) | (inputu == ws));
}

__attribute__((noinline))
bool no_inline_fast_is_special(std::string_view input) {
  uint64_t inputu = string_to_uint64(input);
  if ((inputu & 0xffffffffff) == string_to_uint64("https\0\0\0")) {
    return input.size() == 5;
  }
  if ((inputu & 0xffffffff) == string_to_uint64("http\0\0\0\0")) {
    return input.size() == 4;
  }
  if (uint32_t(inputu) == string_to_uint32("file")) {
    return input.size() == 4;
  }
  if ((inputu & 0xffffff) == string_to_uint32("ftp\0")) {
    return input.size() == 3;
  }
  if ((inputu & 0xffffff) == string_to_uint32("wss\0")) {
    return input.size() == 3;
  }
  if ((inputu & 0xffff) == string_to_uint32("ws\0\0")) {
    return input.size() == 2;
  }
  return false;
}

bool direct_is_special(std::string_view input) {
  return (input == "https") | (input == "http") | (input == "ftp") |
         (input == "file") | (input == "ws") | (input == "wss");
}

bool directb_is_special(std::string_view input) {
  return (input == "https") || (input == "http") || (input == "ftp") ||
         (input == "file") || (input == "ws") || (input == "wss");
}

__attribute__((noinline))
bool no_inline_direct_is_special(std::string_view input) {
  return (input == "https") | (input == "http") | (input == "ftp") |
         (input == "file") | (input == "ws") | (input == "wss");
}

__attribute__((noinline))
bool no_inline_directb_is_special(std::string_view input) {
  return (input == "https") || (input == "http") || (input == "ftp") ||
         (input == "file") || (input == "ws") || (input == "wss");
}

static const std::unordered_set<std::string_view> special_set = {
    "ftp", "file", "http", "https", "ws", "wss"};
bool hash_is_special(std::string_view input) {
  return special_set.find(input) != special_set.end();
}

__attribute__((noinline))
bool no_inline_hash_is_special(std::string_view input) {
  return special_set.find(input) != special_set.end();
}

std::vector<std::string_view> populate(size_t length) {
  std::mt19937 gen;
  // we generate a distribution where https is more common
  std::discrete_distribution<> d({50, 20, 10, 10, 5, 5, 5, 5, 5, 5});
  const static char* options[] = {
      "https\0\0\0",    "http\0\0\0\0",  "ftp\0\0\0\0\0", "file\0\0\0\0",
      "ws\0\0\0\0\0\0", "wss\0\0\0\0\0", "garbage\0",     "fake\0\0\0\0",
      "httpr\0\0\0", "filer\0\0\0"};
  std::vector<std::string_view> answer;
  answer.reserve(length);
  for (size_t pos = 0; pos < length; pos++) {
    const char* picked{options[d(gen)]};
    answer.emplace_back(std::string_view(picked, strlen(picked)));
  }
  return answer;
}

void simulation(size_t N) {
  puts("=======\n");
  printf("Simulation with N = %zu \n", N);


  std::vector<std::string_view> data = populate(N);
  init_dfa_states();
  init_dfa_states2();
  init_mask_data();

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += craftedhash_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("nocheatinghash %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += (Perfect_Hash::in_word_set(v.data(), v.size()) != nullptr);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("gperf %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += dfa(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("dfa %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += dfa2(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("dfa2 %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += dfa3(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("dfa3 %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += mask(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("mask %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += mask2(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("mask2 %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += hash_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("hash_is_special %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += direct_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("direct_is_special %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += directb_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("directb_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += fast_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("fast_is_special %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += fast2_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("fast2_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += mulhi_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("mulhi_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += shiftxor_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("shiftxor_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += dfa_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("dfa_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += dfa3_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("dfa3_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += mask3_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("mask3_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += branchless_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("branchless_is_special %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    const std::regex txt_regex("(https)|(http)|(ftp)|(file)|(ws)|(wss)");
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += std::regex_match(v.begin(), v.end(), txt_regex);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("regex %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    const std::regex txt_regex("https?|ftp|file|wss?");
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += std::regex_match(v.begin(), v.end(), txt_regex);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("regex2 %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_dfa(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_dfa %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_dfa2(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_dfa2 %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_dfa3(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_dfa3 %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_mask(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_mask %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_mask2(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_mask2 %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_hash_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_hash_is_special %f ns/string, matches = %zu \n", t, matches);
  }

  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_direct_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_direct_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_directb_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_directb_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_fast_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_fast_is_special %f ns/string, matches = %zu \n", t, matches);
  }


  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_fast2_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_fast2_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_branchless_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_branchless_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_mulhi_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_mulhi_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_shiftxor_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_shiftxor_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_dfa_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_dfa_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_dfa3_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_dfa3_is_special %f ns/string, matches = %zu \n", t, matches);
  }
  {
    uint64_t start = nano();
    uint64_t finish = start;
    size_t count{0};
    size_t matches{0};
    uint64_t threshold = 500000000;
    for (; finish - start < threshold;) {
      count++;
      matches = 0;
      for (auto v : data) {
        matches += no_inline_mask3_is_special(v);
      }
      finish = nano();
    }
    double t = double(finish - start) / (N * count);

    printf("no_inline_mask3_is_special %f ns/string, matches = %zu \n", t, matches);
  }
}
int main() { simulation(8192); simulation(65536); }
