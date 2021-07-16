#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int int_log2(uint32_t x) { return 31 - __builtin_clz(x|1); }


int digit_count(uint32_t x) {
    static uint32_t table[] = {9, 99, 999, 9999, 99999, 
    999999, 9999999, 99999999, 999999999};
    int y = (9 * int_log2(x)) >> 5;
    y += x > table[y];
    return y + 1;
}

int slow_digit_count(uint32_t x) {
    if(x == 0) { return 1;}
    static uint32_t table[] = {1,      10,      100,      1000,      10000,
                             100000, 1000000, 10000000, 100000000, 1000000000};
    int ans = 9;
    while(x < table[ans]) { ans --;}
    return ans + 1;
    
}

int main() {
  for(uint32_t val = 1; val != 0; val++) {
      if(slow_digit_count(val) != digit_count(val)) {
          printf("%u %d %d \n", val, slow_digit_count(val), digit_count(val));
            return EXIT_FAILURE;
      }
  }
  return EXIT_SUCCESS;
}