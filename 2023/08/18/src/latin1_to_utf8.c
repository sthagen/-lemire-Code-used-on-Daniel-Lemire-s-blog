
#include "latin1_to_utf8.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <x86intrin.h> // update if we need to support Windows.

size_t latin1_to_utf8(const char *buf, size_t len, char *utf8_output) {
  const unsigned char *data = (const unsigned char *)(buf);
  size_t pos = 0;
  char *start = utf8_output;
  while (pos < len) {
    // try to convert the next block of 16 ASCII bytes
    if (pos + 16 <=
        len) { // if it is safe to read 16 more bytes, check that they are ascii
      uint64_t v1;
      memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v =
          v1 | v2; // We are only interested in these bits: 1000 1000 1000 1000,
                   // so it makes sense to concatenate everything
      if ((v & 0x8080808080808080) ==
          0) { // if NONE of these are set, e.g. all of them are zero, then
               // everything is ASCII
        size_t final_pos = pos + 16;
        while (pos < final_pos) {
          *utf8_output++ = (char)(buf[pos]);
          pos++;
        }
        continue;
      }
    }

    unsigned char byte = data[pos];
    if ((byte & 0x80) == 0) { // if ASCII
      // will generate one UTF-8 byte
      *utf8_output++ = (char)(byte);
      pos++;
    } else {
      // will generate two UTF-8 bytes
      *utf8_output++ = (char)((byte >> 6) | 0b11000000);
      *utf8_output++ = (char)((byte & 0b111111) | 0b10000000);
      pos++;
    }
  }
  return (size_t)(utf8_output - start);
}

void printbinary(uint64_t n) {
  for (size_t i = 0; i < 64; i++) {
    if (n & 1)
      printf("1");
    else
      printf("0");

    n >>= 1;
  }
  printf("\n");
}
void print8(const char *name, __m512i x) {
  printf("%.32s : ", name);
  uint8_t buffer[64];
  _mm512_storeu_si512((__m512i *)buffer, x);
  for (size_t i = 0; i < 64; i++) {
    printf("%02x ", buffer[i]);
  }
  printf("\n");
}

size_t latin1_to_utf8_avx512_InstLatX64_2(const char *buf, size_t len, char *utf8_output) {
  char *start = utf8_output;
  size_t pos = 0;
  __m512i concat = _mm512_set1_epi64(0x3036202610160006);
  for (; pos + 32 <= len; pos += 32) {
    __m256i input = _mm256_loadu_si256((__m256i *)(buf + pos));
    uint32_t nonascii = (uint32_t)_mm256_movemask_epi8(input);
    int output_size = 32 + _mm_popcnt_u32(nonascii);
    uint64_t compmask = ~_pdep_u64(~nonascii, UINT64_C(0x5555555555555555));
    __m512i input16   = _mm512_cvtepi8_epi16(input);
    __m512i concat16  = _mm512_multishift_epi64_epi8(concat, input16);
    __m512i output16  = _mm512_ternarylogic_epi32(concat16, _mm512_and_epi32(input16, _mm512_set1_epi32(0x40004000)), _mm512_set1_epi32((int)0xffc3ffc3), 0x20);
    __m512i output    = _mm512_maskz_compress_epi8(compmask, output16);
    __mmask64 write_mask = _bzhi_u64(~0ULL, (long long unsigned int)output_size);
    _mm512_mask_storeu_epi8(utf8_output, write_mask, output);
    utf8_output += output_size;
  }
  if (pos < len) {
    __mmask32 load_mask = _bzhi_u32(~0U, (unsigned int)(len - pos));
    __m256i input = _mm256_maskz_loadu_epi8(load_mask, (__m256i *)(buf + pos));
    uint32_t nonascii = (uint32_t)_mm256_movemask_epi8(input);
    long long unsigned int output_size =
        (long long unsigned int)(len - pos + (size_t)_mm_popcnt_u32(nonascii));
    uint64_t compmask = ~_pdep_u64(~nonascii, UINT64_C(0x5555555555555555));
    __m512i input16   = _mm512_cvtepi8_epi16(input);
    __m512i concat16  = _mm512_multishift_epi64_epi8(concat, input16);
    __m512i output16  = _mm512_ternarylogic_epi32(concat16, _mm512_and_epi32(input16, _mm512_set1_epi32(0x40004000)), _mm512_set1_epi32((int)0xffc3ffc3), 0x20);
    __m512i output    = _mm512_maskz_compress_epi8(compmask, output16);
    __mmask64 write_mask = _bzhi_u64(~0ULL, output_size);
    _mm512_mask_storeu_epi8(utf8_output, write_mask, output);
    utf8_output += output_size;
  }
  return (size_t)(utf8_output - start);
}


size_t latin1_to_utf8_avx512_InstLatX64(const char *buf, size_t len, char *utf8_output) {
  char *start = utf8_output;
  size_t pos = 0;
  __m512i byteflip = _mm512_setr_epi64(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                       0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                       0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                       0x0607040502030001, 0x0e0f0c0d0a0b0809);
  // Note that
  for (; pos + 32 <= len; pos += 32) {
    __m256i input = _mm256_loadu_si256((__m256i *)(buf + pos));
    __mmask32 nonascii = _mm256_movepi8_mask(input);
    int output_size = 32 + _popcnt32(nonascii);
    uint64_t compmask = ~_pdep_u64(~nonascii, UINT64_C(0x5555555555555555));
    __mmask32 sixth =
        _mm256_cmpge_epu8_mask(input, _mm256_set1_epi8((char)192));
    __m512i input16 = _mm512_cvtepu8_epi16(input);
        input16 =_mm512_add_epi16(input16, _mm512_set1_epi16((short int)0xc200));
    __m512i output16 =
        _mm512_mask_add_epi16(input16, sixth, input16,
                              _mm512_set1_epi16(0x00c0));
    output16 = _mm512_shuffle_epi8(output16, byteflip);
    __m512i output = _mm512_maskz_compress_epi8(compmask, output16);
    __mmask64 write_mask = _bzhi_u64(~0ULL, (long long unsigned int)output_size);
    _mm512_mask_storeu_epi8(utf8_output, write_mask, output);
    utf8_output += output_size;
  }
  // We repeat the code, this could be reengineered to be nicer.
  if (pos < len) {
    __mmask32 load_mask = _bzhi_u32(~0U, (unsigned int)(len - pos));
    __m256i input = _mm256_maskz_loadu_epi8(load_mask, (__m256i *)(buf + pos));
    __mmask32 nonascii = _mm256_movepi8_mask(input);
    long long unsigned int output_size =
        (long long unsigned int)(len - pos + (size_t)_popcnt32(nonascii));

    uint64_t compmask = ~_pdep_u64(~nonascii, UINT64_C(0x5555555555555555));
    __mmask32 sixth =
        _mm256_cmpge_epu8_mask(input, _mm256_set1_epi8((char)192));
    __m512i input16 = _mm512_cvtepu8_epi16(input);
    input16 =_mm512_add_epi16(input16, _mm512_set1_epi16((short int)0xc200));
     __m512i output16 =   _mm512_mask_add_epi16(input16, sixth, input16,
                              _mm512_set1_epi16(0x00c0));
     output16 = _mm512_shuffle_epi8(output16, byteflip);
    __m512i output = _mm512_maskz_compress_epi8(compmask, output16);

    __mmask64 write_mask = _bzhi_u64(~0ULL, output_size);
    _mm512_mask_storeu_epi8(utf8_output, write_mask, output);
    utf8_output += output_size;
  }
  return (size_t)(utf8_output - start);
}


size_t latin1_to_utf8_avx512(const char *buf, size_t len, char *utf8_output) {
  char *start = utf8_output;
  size_t pos = 0;
  for (; pos + 32 <= len; pos += 32) {
    __m256i input = _mm256_loadu_si256((__m256i *)(buf + pos));
    __mmask32 nonascii = _mm256_movepi8_mask(input);
    int output_size = 32 + _mm_popcnt_u32(nonascii);
    uint64_t compmask = ~_pdep_u64(~nonascii, UINT64_C(0x5555555555555555));
    __m512i input16   = _mm512_cvtepi8_epi16(input);
    __m512i concat16  = _mm512_shrdi_epi16(_mm512_slli_epi16(input16, 2), input16, 8);
    __m512i output16  = _mm512_ternarylogic_epi32(concat16, _mm512_and_epi32(input16, _mm512_set1_epi32(0x40004000)), _mm512_set1_epi32((int)0xffc3ffc3), 0x20);
    __m512i output    = _mm512_maskz_compress_epi8(compmask, output16);
    __mmask64 write_mask = _bzhi_u64(~0ULL, (long long unsigned int)output_size);
    _mm512_mask_storeu_epi8(utf8_output, write_mask, output);
    utf8_output += output_size;
  }
  // We repeat the code, this could be reengineered to be nicer.
  if (pos < len) {
    __mmask32 load_mask = _bzhi_u32(~0U, (unsigned int)(len - pos));
    __m256i input = _mm256_maskz_loadu_epi8(load_mask, (__m256i *)(buf + pos));
    __mmask32 nonascii = _mm256_movepi8_mask(input);
    long long unsigned int output_size =
        (long long unsigned int)(len - pos + (size_t)_mm_popcnt_u32(nonascii));
    uint64_t compmask = ~_pdep_u64(~nonascii, UINT64_C(0x5555555555555555));
    __m512i input16   = _mm512_cvtepi8_epi16(input);
    __m512i concat16  = _mm512_shrdi_epi16(_mm512_slli_epi16(input16, 2), input16, 8);
    __m512i output16  = _mm512_ternarylogic_epi32(concat16, _mm512_and_epi32(input16, _mm512_set1_epi32(0x40004000)), _mm512_set1_epi32((int)0xffc3ffc3), 0x20);
    __m512i output    = _mm512_maskz_compress_epi8(compmask, output16);
    __mmask64 write_mask = _bzhi_u64(~0ULL, output_size);
    _mm512_mask_storeu_epi8(utf8_output, write_mask, output);
    utf8_output += output_size;
  }
  return (size_t)(utf8_output - start);
}




static inline size_t latin1_to_utf8_avx512_vec(__m512i input, size_t input_len, char *utf8_output, int mask_output) {
  __mmask64 nonascii = _mm512_movepi8_mask(input);
  size_t output_size = input_len + (size_t)_popcnt64(nonascii);
  
  __mmask64 sixth =
      _mm512_cmpge_epu8_mask(input, _mm512_set1_epi8(-64));
  
  const uint64_t alternate_bits = UINT64_C(0x5555555555555555);
  uint64_t ascii = ~nonascii;
  uint64_t maskA = ~_pdep_u64(ascii, alternate_bits);
  uint64_t maskB = ~_pdep_u64(ascii>>32, alternate_bits);
  
  // interleave bytes from top and bottom halves (abcd...ABCD -> aAbBcCdD)
  __m512i input_interleaved = _mm512_permutexvar_epi8(_mm512_set_epi32(
    0x3f1f3e1e, 0x3d1d3c1c, 0x3b1b3a1a, 0x39193818,
    0x37173616, 0x35153414, 0x33133212, 0x31113010,
    0x2f0f2e0e, 0x2d0d2c0c, 0x2b0b2a0a, 0x29092808,
    0x27072606, 0x25052404, 0x23032202, 0x21012000
  ), input);
  
  // double size of each byte, and insert the leading byte
  __m512i outputA = _mm512_shldi_epi16(input_interleaved, _mm512_set1_epi8(-62), 8);
  outputA = _mm512_mask_add_epi16(outputA, (__mmask32)sixth, outputA, _mm512_set1_epi16(1 - 0x4000));
  
  __m512i leadingB = _mm512_mask_blend_epi16((__mmask32)(sixth>>32), _mm512_set1_epi16(0x00c2), _mm512_set1_epi16(0x40c3));
  __m512i outputB = _mm512_ternarylogic_epi32(input_interleaved, leadingB, _mm512_set1_epi16((short)0xff00), (240 & 170) ^ 204); // (input_interleaved & 0xff00) ^ leadingB
  
  // prune redundant bytes
  outputA = _mm512_maskz_compress_epi8(maskA, outputA);
  outputB = _mm512_maskz_compress_epi8(maskB, outputB);
  
  
  size_t output_sizeA = (size_t)_popcnt32((uint32_t)nonascii) + 32;
  if(mask_output) {
    if(input_len > 32) { // is the second half of the input vector used?
      __mmask64 write_mask = _bzhi_u64(~0ULL, output_sizeA);
      _mm512_mask_storeu_epi8(utf8_output, write_mask, outputA);
      utf8_output += output_sizeA;
      write_mask = _bzhi_u64(~0ULL, output_size - output_sizeA);
      _mm512_mask_storeu_epi8(utf8_output, write_mask, outputB);
    } else {
      __mmask64 write_mask = _bzhi_u64(~0ULL, output_size);
      _mm512_mask_storeu_epi8(utf8_output, write_mask, outputA);
    }
  } else {
    _mm512_storeu_si512(utf8_output, outputA);
    utf8_output += output_sizeA;
    _mm512_storeu_si512(utf8_output, outputB);
  }
  return output_size;
}
 
// if the likelihood of non-ASCII characters is low, it may make sense to add a branch for a faster routine
static inline size_t latin1_to_utf8_avx512_branch(__m512i input, char *utf8_output, int br) {
  __mmask64 nonascii = _mm512_movepi8_mask(input);
  size_t nonascii_count = (size_t)_popcnt64(nonascii);
  
  if(br == 0) { // shortcut for no non-ASCII characters
    if(nonascii_count > 0)
      return latin1_to_utf8_avx512_vec(input, 64, utf8_output, 0);
    _mm512_storeu_si512(utf8_output, input);
    return 64;
    
  } else { // shortcut for up to 1 non-ASCII characters
    if(nonascii_count > 1)
      return latin1_to_utf8_avx512_vec(input, 64, utf8_output, 0);
    
    __mmask64 sixth =
        _mm512_cmpge_epu8_mask(input, _mm512_set1_epi8(-64));
    input = _mm512_mask_add_epi8(input, sixth, input, _mm512_set1_epi8(-64));
    
    // we're writing either 64 or 65 bytes; write the last byte to cater for the latter case (the earlier bytes will get overwritten)
    //_mm512_storeu_si512(utf8_output + 1, input);
    utf8_output[64] = (char)_mm_extract_epi8(_mm512_extracti32x4_epi32(input, 3), 15);
    
    __m512i leading = _mm512_mask_blend_epi8(sixth, _mm512_set1_epi8(-62), _mm512_set1_epi8(-61));
    input = _mm512_mask_expand_epi8(leading, ~nonascii, input);
    _mm512_storeu_si512(utf8_output, input);
    
    return 64 + nonascii_count;
  }
}
 
size_t latin1_to_utf8_avx512_my_nobranch(const char *buf, size_t len, char *utf8_output) {
  char *start = utf8_output;
  size_t pos = 0;
  // if there's at least 128 bytes remaining, we don't need to mask the output
  for (; pos + 128 <= len; pos += 64) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_vec(input, 64, utf8_output, 0);
  }
  // in the last 128 bytes, the first 64 may require masking the output
  if (pos + 64 <= len) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_vec(input, 64, utf8_output, 1);
    pos += 64;
  }
  // with the last 64 bytes, the input also needs to be masked
  if (pos < len) {
    __mmask64 load_mask = _bzhi_u64(~0ULL, (unsigned int)(len - pos));
    __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_vec(input, len - pos, utf8_output, 1);
  }
  return (size_t)(utf8_output - start);
}
 
size_t latin1_to_utf8_avx512_my_branch0(const char *buf, size_t len, char *utf8_output) {
  char *start = utf8_output;
  size_t pos = 0;
  // if there's at least 128 bytes remaining, we don't need to mask the output
  for (; pos + 128 <= len; pos += 64) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_branch(input, utf8_output, 0);
  }
  // in the last 128 bytes, the first 64 may require masking the output
  if (pos + 64 <= len) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_vec(input, 64, utf8_output, 1);
    pos += 64;
  }
  // with the last 64 bytes, the input also needs to be masked
  if (pos < len) {
    __mmask64 load_mask = _bzhi_u64(~0ULL, (unsigned int)(len - pos));
    __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_vec(input, len - pos, utf8_output, 1);
  }
  return (size_t)(utf8_output - start);
}
 
size_t latin1_to_utf8_avx512_my_branch1(const char *buf, size_t len, char *utf8_output) {
  char *start = utf8_output;
  size_t pos = 0;
  // if there's at least 128 bytes remaining, we don't need to mask the output
  for (; pos + 128 <= len; pos += 64) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_branch(input, utf8_output, 1);
  }
  // in the last 128 bytes, the first 64 may require masking the output
  if (pos + 64 <= len) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_vec(input, 64, utf8_output, 1);
    pos += 64;
  }
  // with the last 64 bytes, the input also needs to be masked
  if (pos < len) {
    __mmask64 load_mask = _bzhi_u64(~0ULL, (unsigned int)(len - pos));
    __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_vec(input, len - pos, utf8_output, 1);
  }
  return (size_t)(utf8_output - start);
}