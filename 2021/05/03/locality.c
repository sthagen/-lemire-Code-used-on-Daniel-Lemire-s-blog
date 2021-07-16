#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

__attribute__((noinline)) void increment(uint8_t *array, uint32_t *random,
                                         size_t hits) {
  for (size_t i = 0; i < hits; i++) {
    array[random[i]]++;
  }
}
#if defined(__x86_64) || defined(__i386__)
void array_cache_flush(uint8_t *B, size_t length) {
  const int32_t CACHELINESIZE = 64; // 64 bytes per cache line
  for (size_t k = 0; k < length; k += CACHELINESIZE / sizeof(uint8_t)) {
    __builtin_ia32_clflush(B + k);
  }
}
#else
void array_cache_flush(uint8_t *, size_t) {}
#endif

int main(int argc, const char **val) {
  size_t N = 10000000;
  if (argc > 1) {
    N = atoll(val[1]);
  }
  printf("N = %zu, %.1f MB\n", N, N / (1024. * 1024));
  uint8_t *array = (uint8_t *)malloc(N * sizeof(uint8_t));
  for (size_t i = 0; i < N; i++) {
    array[i] = i;
  }
  uint32_t *random = (uint32_t *)malloc(N * sizeof(uint32_t));
  uint32_t *random2 = (uint32_t *)malloc(N * sizeof(uint32_t));

  for (uint32_t gap = 32; gap <= 32768; gap *= 2) {
    printf("starting experiments with gap = %u.\n", gap);
    struct timespec start, stop;
    size_t total_trials = 20;

    size_t ns2 = 0;
    size_t ns2p = 0;
    size_t ns2_redo = 0;
    size_t ns2p_redo = 0;
    for (size_t trial = 0; trial < total_trials; trial++) {
      for (size_t i = 0; i < N; i++) {
        random[i] = rand() % N;
      }
      for (size_t i = 0; i < N / 2; i++) {
        random2[2 * i] = rand() % N;
        random2[2 * i + 1] = random2[2 * i] + (rand() % gap);
        // We reflect back if we exceed [0,N).
        if (random2[2 * i + 1] >= N) {
          random2[2 * i + 1] = N - (random2[2 * i + 1] - N) - 1;
        }
      }
      array_cache_flush(array, N);
      clock_gettime(CLOCK_REALTIME, &start);
      increment(array, random2, N);
      clock_gettime(CLOCK_REALTIME, &stop);
      ns2p += (stop.tv_sec - start.tv_sec) * 1000000000 +
              (stop.tv_nsec - start.tv_nsec);
      array_cache_flush(array, N);
      clock_gettime(CLOCK_REALTIME, &start);
      increment(array, random, N);
      clock_gettime(CLOCK_REALTIME, &stop);
      ns2 += (stop.tv_sec - start.tv_sec) * 1000000000 +
             (stop.tv_nsec - start.tv_nsec);

      array_cache_flush(array, N);
      clock_gettime(CLOCK_REALTIME, &start);
      increment(array, random2, N);
      clock_gettime(CLOCK_REALTIME, &stop);
      ns2p_redo += (stop.tv_sec - start.tv_sec) * 1000000000 +
                   (stop.tv_nsec - start.tv_nsec);
      array_cache_flush(array, N);
      clock_gettime(CLOCK_REALTIME, &start);
      increment(array, random, N);
      clock_gettime(CLOCK_REALTIME, &stop);
      ns2_redo += (stop.tv_sec - start.tv_sec) * 1000000000 +
                  (stop.tv_nsec - start.tv_nsec);
    }
    printf("random   : %.3f ns\n", (double)ns2 / N / total_trials);
    printf("localized  : %.3f ns\n", (double)ns2p / N / total_trials);
    printf("difference : %.1f %% \n", ((double)ns2 / (double)ns2p * 100) - 100);
    printf("random (redo)    : %.3f ns\n", (double)ns2_redo / N / total_trials);
    printf("localized (redo) : %.3f ns\n",
           (double)ns2p_redo / N / total_trials);

    printf("difference : %.1f %% \n",
           ((double)ns2_redo / (double)ns2p_redo * 100) - 100);
  }

  free(array);
  free(random);
  free(random2);

  return EXIT_SUCCESS;
}
