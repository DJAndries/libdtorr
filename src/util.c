#include "util.h"

unsigned int bigend_to_uint(char* buf) {
  int i;
  unsigned int result = 0;
  for (i = 3; i >= 0; i--) {
    result <<= 8;
    result |= buf[i];
  }
  return result;
}