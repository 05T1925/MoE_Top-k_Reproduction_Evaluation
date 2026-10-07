/*
 * Deterministic getrandom() source for paired TEST_ONLY counter-overhead runs.
 * This is deliberately not cryptographic and must never be used by a party
 * protocol, key-generation path outside TEST_ONLY, or security evaluation.
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/random.h>

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t g_counter = UINT64_C(0x53414d504c455331);
static uint64_t g_word = 0;
static unsigned int g_used = 8;

static uint64_t splitmix64_next(void) {
  uint64_t z = (g_counter += UINT64_C(0x9e3779b97f4a7c15));
  z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
  return z ^ (z >> 31);
}

ssize_t getrandom(void *buffer, size_t length, unsigned int flags) {
  (void)flags;
  if (length == 0) return 0;
  if (buffer == NULL) return -1;
  pthread_mutex_lock(&g_mutex);
  unsigned char *out = (unsigned char *)buffer;
  for (size_t i = 0; i < length; ++i) {
    if (g_used == 8) {
      g_word = splitmix64_next();
      g_used = 0;
    }
    out[i] = (unsigned char)(g_word >> (8U * g_used));
    ++g_used;
  }
  pthread_mutex_unlock(&g_mutex);
  return (ssize_t)length;
}
