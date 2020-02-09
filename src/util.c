#include "util.h"
#include <sys/time.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

unsigned int bigend_to_uint(char* buf) {
  int i;
  unsigned int result = 0;
  for (i = 0; i < 4; i++) {
    result <<= 8;
    result |= buf[i] & 0xFF;
  }
  return result;
}

void uint_to_bigend(char* buf, unsigned int val) {
  int i;
  for (i = 3; i >= 0; i--) {
    buf[i] = val & 0xFF;
    val >>= 8;
  }
}

unsigned long get_time_ms() {
  struct timeval curr_time;
  gettimeofday(&curr_time, 0);
  return (curr_time.tv_sec * 1000) + (curr_time.tv_usec / 1000);
}

void dsleep(unsigned long ms) {
  #ifndef _WIN32
    usleep(ms * 1000);
  #else
    Sleep(ms);
  #endif
}

unsigned long calc_piece_length(unsigned long piece_count, unsigned long piece_length, unsigned long total_length, unsigned long index) {
  return (index + 1) == piece_count ? total_length - (index * piece_length) : piece_length;
}