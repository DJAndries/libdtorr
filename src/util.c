#include "util.h"
#include <sys/time.h>

unsigned int bigend_to_uint(char* buf) {
  int i;
  unsigned int result = 0;
  for (i = 3; i >= 0; i--) {
    result <<= 8;
    result |= buf[i];
  }
  return result;
}

void uint_to_bigend(char* buf, unsigned int val) {
  int i;
  for (i = 0; i < 4; i++) {
    buf[i] = val & 0xFF;
    val >>= 8;
  }
}

unsigned long get_time_ms() {
  struct timeval curr_time;
  gettimeofday(&curr_time, 0);
  return (curr_time.tv_sec * 1000) + (curr_time.tv_usec / 1000);
}