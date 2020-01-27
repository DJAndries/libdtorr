#include "dtorr/dtorr.h"
#include "dtorr/bencoding_encode.h"
#include "dtorr/bencoding_decode.h"
#include <stdio.h>
#include <stdlib.h>

#define TEST_COUNT 6

char *tests[] = {
  "i-3e",
  "11:Hello hello",
  "l1:a2:aa3:aaae",
  "d1:b5:hello1:a6:hello2e",
  "d1:b5:hello1:al1:a2:bbee",
  "d1:b5:h\x00\x00lo1:al1:a2:bbee"
};

unsigned long tests_len[] = {
  4,
  14,
  14,
  23,
  24,
  24
};

void print_encoded(char* content, unsigned long length) {
  unsigned long i;
  char c;
  for (i = 0; i < length; i++) {
    c = content[i];

    if (c < ' ') {
      printf("%x", c);
    } else {
      printf("%c", c);
    }
  }
  printf("\n");
}

int main() {
  unsigned long i;
  dtorr_config* config = (dtorr_config*)malloc(sizeof(dtorr_config));
  char* str;
  char* test;
  dtorr_node* node;
  unsigned long encode_length;

  config->log_level = 4;
  config->log_handler = 0;

  for (i = 0; i < TEST_COUNT; i++) {
    test = tests[i];
    printf("Decode test: ");
    print_encoded(test, tests_len[i]);
    node = bencoding_decode(config, test, tests_len[i]);
    if (node == 0) {
      return 1;
    }

    printf("Decoded. Encoding...\n");
    str = bencoding_encode(config, node, &encode_length);
    if (str == 0) {
      return 1;
    }
    printf("Encode test: ");
    print_encoded(str, encode_length);
  }

  printf("Done!\n");

  return 0;
}