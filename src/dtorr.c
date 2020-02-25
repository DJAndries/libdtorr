#include "dtorr/dtorr.h"
#include "dsock.h"
#include <stdlib.h>

int dtorr_init(dtorr_config* config) {
  return dsock_init();
}

void mfree(void* ptr) {
  free(ptr);
}
