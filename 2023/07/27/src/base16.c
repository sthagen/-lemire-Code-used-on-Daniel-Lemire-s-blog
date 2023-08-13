
#include "base16.h"

#include <stdint.h>
#include <x86intrin.h> // update if we need to support Windows.

/// Source: Faster Base64 Encoding and Decoding Using AVX2 Instructions,
///         https://arxiv.org/abs/1704.00605
size_t base16hex_simd(uint8_t *dst, const uint8_t *src) {
  //static int8_t zero_masks[32] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  //                                0,  0,  0,  0,  0,  -1, -1, -1, -1, -1, -1,
  //                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  bool valid = true;
  const __m128i delta_check = _mm_setr_epi8(-16, -32, -47, 71, 58, -96, 26,
                                            -128, 0, 0, 0, 0, 0, 0, 0, 0);
  const __m128i delta_rebase =
      _mm_setr_epi8(0, 0, -47, -47, -54, 0, -86, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  const uint8_t *srcinit = src;
  do {
    __m128i v = _mm_loadu_si128((__m128i *)src);
    __m128i vm1 = _mm_add_epi8(v, _mm_set1_epi8(-1));
    __m128i hash_key =
        _mm_and_si128(_mm_srli_epi32(vm1, 4), _mm_set1_epi8(0x0F));
    __m128i check = _mm_add_epi8(_mm_shuffle_epi8(delta_check, hash_key), vm1);
    v = _mm_add_epi8(vm1, _mm_shuffle_epi8(delta_rebase, hash_key));
    unsigned int m = (unsigned)_mm_movemask_epi8(check);
    if (m) {
      int length = __builtin_ctzll(m);
      if (length == 0) {
        break;
      }
      src += length;
      //__m128i zero_mask =
      //    _mm_loadu_si128((__m128i *)(zero_masks + 16 - length));
      //v = _mm_andnot_si128(zero_mask, v);
      valid = false;
    } else { // common case
      src += 16;
    }
    // the maddubs can be replaced by a shift, an or and an and (so 3
    // instructions)
    //
    // const __m128i t1 = _mm_srli_epi32(v, 4);
    // const __m128i t3 = _mm_or_si128(t0, t1);
    // const __m128i t4 = _mm_and_si128(t3, _mm_set1_epi16(0x00ff));
    //
    const __m128i t3 = _mm_maddubs_epi16(v, _mm_set1_epi16(0x0110));
    // the shuffle can be replaced by a pack instruction, e.g.,
     const __m128i t5 = _mm_packus_epi16(t3, t3);
    //const __m128i t5 =
    //    _mm_shuffle_epi8(t3, _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1,
    //                                       -1, -1, -1, -1, -1, -1));
    _mm_storeu_si128((__m128i *)dst, t5);
    // could use _mm_storel_epi64 but will likely be slower
    // _mm_storel_epi64((__m64*)dst, t5);
    dst += 8;
  } while (valid);
  return (size_t)(src - srcinit);
}

/// Source: Faster Base64 Encoding and Decoding Using AVX2 Instructions,
///         https://arxiv.org/abs/1704.00605
size_t base16hex_avx(uint8_t *dst, const uint8_t *src) {
  //static int8_t zero_masks[64] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  //                                0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  //                                0,  0,  0,  0,  0,  -1, -1, -1, -1, -1, -1,
  //                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1, -1, -1,
  //                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  bool valid = true;
  const __m256i delta_check = _mm256_setr_epi8(-16, -32, -47, 71, 58, -96, 26,
                                            -128, 0, 0, 0, 0, 0, 0, 0, 0,
                                            -16, -32, -47, 71, 58, -96, 26,
                                            -128, 0, 0, 0, 0, 0, 0, 0, 0);
  const __m256i delta_rebase =
      _mm256_setr_epi8(0, 0, -47, -47, -54, 0, -86, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, -47, -47, -54, 0, -86, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  const uint8_t *srcinit = src;
  do {
    __m256i v = _mm256_loadu_si256((__m256i *)src);
    __m256i vm1 = _mm256_add_epi8(v, _mm256_set1_epi8(-1));
    __m256i hash_key =
        _mm256_and_si256(_mm256_srli_epi32(vm1, 4), _mm256_set1_epi8(0x0F));
    __m256i check = _mm256_add_epi8(_mm256_shuffle_epi8(delta_check, hash_key), vm1);
    v = _mm256_add_epi8(vm1, _mm256_shuffle_epi8(delta_rebase, hash_key));
    unsigned int m = (unsigned)_mm256_movemask_epi8(check);
    if (m) {
      int length = __builtin_ctzll(m);
      if (length == 0) {
        break;
      }
      src += length;
      //__m256i zero_mask =
      //    _mm256_loadu_si256((__m256i *)(zero_masks + 32 - length));
      //v = _mm256_andnot_si256(zero_mask, v);
      valid = false;
    } else { // common case
      src += 32;
    }
    const __m256i t3 = _mm256_maddubs_epi16(v, _mm256_set1_epi16(0x0110));
    const __m256i t5 = _mm256_permute4x64_epi64(_mm256_packus_epi16(t3, t3), 0b11011000);
    _mm_storeu_si128((__m128i *)dst, _mm256_castsi256_si128(t5));
    dst += 16;
  } while (valid);
  return (size_t)(src - srcinit);
}

// Returns the length of the continuous sequence found, but only process an even
// number of characters, the caller is responsible for processing the leftover
// character (if any). If leftoverin is not the null pointer, then we take
// *leftoverin as the first character of the sequence.
static size_t base16hex_simd_justeven(uint8_t *dst, const uint8_t *src,
                                      const uint8_t *leftoverin) {
  //static int8_t zero_masks[32] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  //                                0,  0,  0,  0,  0,  -1, -1, -1, -1, -1, -1,
  //                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  bool valid = true;
  const __m128i delta_check = _mm_setr_epi8(-16, -32, -47, 71, 58, -96, 26,
                                            -128, 0, 0, 0, 0, 0, 0, 0, 0);
  const __m128i delta_rebase =
      _mm_setr_epi8(0, 0, -47, -47, -54, 0, -86, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  const uint8_t *srcinit = src;

  __m128i v = _mm_loadu_si128((__m128i *)src);
  if (leftoverin) {
    src -= 1;
    v = _mm_alignr_epi8(v, _mm_set1_epi8((char)*leftoverin), 15);
  }

  do {
    __m128i vm1 = _mm_add_epi8(v, _mm_set1_epi8(-1));
    __m128i hash_key =
        _mm_and_si128(_mm_srli_epi32(vm1, 4), _mm_set1_epi8(0x0F));
    __m128i check = _mm_add_epi8(_mm_shuffle_epi8(delta_check, hash_key), vm1);
    v = _mm_add_epi8(vm1, _mm_shuffle_epi8(delta_rebase, hash_key));
    unsigned int m = (unsigned)_mm_movemask_epi8(check);
    if (m) {
      int length = __builtin_ctzll(m);
      if (length == 0) {
        break;
      }
      src += length;
      //__m128i zero_mask =
      //    _mm_loadu_si128((__m128i *)(zero_masks + 16 - (length & ~1)));
      //v = _mm_andnot_si128(zero_mask, v);
      valid = false;
    } else { // common case
      src += 16;
    }
    // the maddubs can be replaced by a shift, an or and an and (so 3
    // instructions)
    //
    // const __m128i t1 = _mm_srli_epi32(v, 4);
    // const __m128i t3 = _mm_or_si128(t0, t1);
    // const __m128i t4 = _mm_and_si128(t3, _mm_set1_epi16(0x00ff));
    //
    const __m128i t3 = _mm_maddubs_epi16(v, _mm_set1_epi16(0x0110));
    // the shuffle can be replaced by a pack instruction, e.g.,
    // const __m128i t5 = _mm_packus_epi16(t3, t3);
    const __m128i t5 =
        _mm_shuffle_epi8(t3, _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1,
                                           -1, -1, -1, -1, -1, -1));
    _mm_storeu_si128((__m128i *)dst, t5);
    dst += 8;
    if (valid) {
      v = _mm_loadu_si128((__m128i *)src);
    } else {
      break;
    }
  } while (true);
  return (size_t)(src - srcinit);
}

static inline bool is_space(uint8_t c) {
  static const bool table[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  return table[(unsigned)c];
}
static inline bool is_hex(uint8_t c) {
  static const bool table[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  return table[(unsigned)c];
}

size_t base16hex_simd_skipspace(uint8_t *dst, const uint8_t *src,
                                size_t *output_size) {
  uint8_t *dstinit = dst;
  const uint8_t *srcinit = src;
  const uint8_t *pointer = NULL;
  {
    size_t consumed = base16hex_simd(dst, src);
    size_t actual_consumed = consumed;
    dst += actual_consumed / 2;
    pointer = (actual_consumed & 1) ? src + consumed - 1 : NULL;
    src += consumed;
  }
  if (is_space(*src)) {
    src++;
    while (is_space(*src)) {
      src++;
    }

    while (is_hex(*src)) {
      size_t consumed = base16hex_simd_justeven(dst, src, pointer);
      size_t actual_consumed = consumed + (pointer == NULL ? 0 : 1);
      dst += actual_consumed / 2;
      pointer = (actual_consumed & 1) ? src + consumed - 1 : NULL;
      src += consumed;
      while (is_space(*src)) {
        src++;
      }
    }
  }

  *output_size = (size_t)(dst - dstinit);
  return (size_t)(src - srcinit);
}

// Parsing hex numbers with validation
// http://0x80.pl/notesen/2022-01-17-validating-hex-parse.html
size_t base16hex_simd_geoff(uint8_t *dst, const uint8_t *src) {
  //static int8_t zero_masks[32] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  //                                0,  0,  0,  0,  0,  -1, -1, -1, -1, -1, -1,
  //                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  bool valid = true;
  const uint8_t *srcinit = src;

  do {
    __m128i v = _mm_loadu_si128((const __m128i *)src);
    const __m128i t1 = _mm_add_epi8(
        v, _mm_set1_epi8((
               char)(0xff - '9'))); // puts 0-9 at top of unsigned integer range
    const __m128i t2 =
        _mm_subs_epu8(t1, _mm_set1_epi8(6)); // now, put 6 blank spots above 0-9
    const __m128i t3 = _mm_sub_epi8(t2, _mm_set1_epi8((char)(0xf0)));
    const __m128i t4 =
        _mm_and_si128(v, _mm_set1_epi8((char)(0xdf))); // squash case
    const __m128i t5 =
        _mm_sub_epi8(t4, _mm_set1_epi8('A')); // put the 'A-F' at 0..5
    const __m128i t6 = _mm_adds_epu8(
        t5, _mm_set1_epi8(10)); // now put A-F at 10..15, with a guarantee that
                                // nothing is smaller
    __m128i t7 = _mm_min_epu8(t3, t6); // t7 is normalized hex nibbles
    const __m128i t8 = _mm_adds_epu8(
        t7,
        _mm_set1_epi8(
            127 - 15)); // t8 has high_bit == 1 iff what we had wasn't valid hex

    unsigned int m = (unsigned)_mm_movemask_epi8(t8);
    if (m) {
      int length = __builtin_ctzll(m);
      if (length == 0) {
        break;
      }
      src += length;
      //__m128i zero_mask =
      //    _mm_loadu_si128((__m128i *)(zero_masks + 16 - length));
      //t7 = _mm_andnot_si128(zero_mask, t7);
      valid = false;
    } else { // common case
      src += 16;
    }

    // now each byte of result have value 0 .. 15, we're going to merge nibbles:
    {
      const __m128i t0 = _mm_maddubs_epi16(t7, _mm_set1_epi16(0x0110));
      const __m128i t1 = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1,
                                       -1, -1, -1, -1, -1);
      const __m128i t3 = _mm_shuffle_epi8(t0, t1);
      _mm_storeu_si128((__m128i *)dst, t3);
      dst += 8;
    }
  } while (valid);
  return (size_t)(src - srcinit);
}

size_t base16hex_simple(uint8_t *dst, const uint8_t *src) {

  static const signed char digittoval[256] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,
      6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1};
  const uint8_t *srcinit = src;
  do {
    // assume that they come in pairs
    int8_t n1 = digittoval[src[0]];
    int8_t n2 = digittoval[src[1]];
    if ((n1 < 0) || (n2 < 0)) {
      break;
    }
    src += 2;
    *dst = (uint8_t)((n1 << 4) | n2);
    dst += 1;
  } while (true);
  return (size_t)(src - srcinit);
}

size_t base16hex_simdzone_fallback(uint8_t *dst, const uint8_t *src) {

  static const uint8_t b16rmap[256] = {
      0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*   0 -   7 */
      0xff, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xff, 0xff, /*   8 -  15 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  16 -  23 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  24 -  31 */
      0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  32 -  39 */
      0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f, /*  40 -  47 */
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /*  48 -  55 */
      0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  56 -  63 */
      0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, /*  64 -  71 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  72 -  79 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  80 -  87 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  88 -  95 */
      0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, /*  96 - 103 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 104 - 111 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 112 - 119 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 120 - 127 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 128 - 135 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 136 - 143 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 144 - 151 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 152 - 159 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 160 - 167 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 168 - 175 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 176 - 183 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 184 - 191 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 192 - 199 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 200 - 207 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 208 - 215 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 216 - 223 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 224 - 231 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 232 - 239 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 240 - 247 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 248 - 255 */
  };
  const uint8_t *zone_b16rmap = b16rmap;
  static const uint8_t b16rmap_special = 0xf0;
  uint32_t state = 0;
  const char *p = (const char *)src;

  for (;; p++) {
    const uint8_t ofs = zone_b16rmap[(uint8_t)*p];

    if (ofs >= b16rmap_special)
      break;

    if (state == 0)
      *dst = (uint8_t)(ofs << 4);
    else {
      *dst |= ofs;
      dst++;
    }

    state = !state;
  }

  return (size_t)((const uint8_t *)p - src);
}