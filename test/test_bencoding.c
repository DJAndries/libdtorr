#include "dtorr/dtorr.h"
#include "dtorr/bencoding_encode.h"
#include "dtorr/bencoding_decode.h"
#include <stdio.h>
#include <stdlib.h>

#define TEST_COUNT 5

char *tests[] = {
  "i-3e",
  "11:Hello hello",
  "l1:a2:aa3:aaae",
  "d1:b5:hello1:a6:hello2e",
  "d1:b5:hello1:al1:a2:bee"
};

int main() {
  unsigned long i;
  dtorr_config* config = (dtorr_config*)malloc(sizeof(dtorr_config));
  char* str;
  char* test;
  dtorr_node* node;

  config->log_level = 1;
  config->log_handler = 0;

  for (i = 0; i < TEST_COUNT; i++) {
    test = tests[i];
    printf("Decode test: %s\n", test);
    node = bencoding_decode(config, test);
    if (node == 0) {
      return 1;
    }

    printf("Decoded. Encoding...\n");
    str = bencoding_encode(config, node);
    if (str == 0) {
      return 1;
    }
    printf("Encode test: %s\n", str);
  }

  printf("Done!\n");

  return 0;
}