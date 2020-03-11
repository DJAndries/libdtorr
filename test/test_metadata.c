#include <stdio.h>
#include <stdlib.h>
#include "dtorr/structs.h"
#include "dtorr/metadata.h"
#include "util.h"

#define TORRENT_COUNT 2

void print_files(dtorr_torrent* torrent) {
  unsigned long long i;
  dtorr_file* file;
  for (i = 0; i < torrent->file_count; i++) {
    file = torrent->files[i];
    printf("File: %s Length: " SPEC_LLU "\n", file->cat_path, file->length);
  }
}

int test_torrent(unsigned int torrent_index) {
  dtorr_config* config = (dtorr_config*)malloc(sizeof(dtorr_config));
  dtorr_torrent* torrent;
  long long size;
  unsigned long long i;
  char filename[256];
  sprintf(filename, "test/torrents/%u.torrent", torrent_index);
  FILE* fp = fopen(filename, "rb");
  char* contents;

  if (fp == 0) {
    fprintf(stderr, "Could not open torrent file\n");
    return 1;
  }

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  contents = (char*)malloc(size + 1);
  fread(contents, 1, size, fp);
  fclose(fp);

  contents[size] = 0;

  config->log_level = 1;
  config->log_handler = 0;

  torrent = load_torrent_metadata(config, contents, (unsigned long long)size);
  if (torrent == 0) {
    fprintf(stderr, "Failed to load torrent!\n");
    return 2;
  }

  printf("Torrent name: %s\n", torrent->name);
  printf("Announce URL: %s\n", torrent->announce);
  printf("Length: " SPEC_LLU "\n", torrent->length);
  printf("Piece length: " SPEC_LLU "\n", torrent->piece_length);
  printf("Piece count: " SPEC_LLU "\n", torrent->piece_count);
  printf("File count: " SPEC_LLU "\n", torrent->file_count);
  if (torrent->files != 0) {
    printf("Has files!\n");
    print_files(torrent);
  }
  printf("Hash: ");

  for (i = 0; i < 20; i++) {
    printf("%02X", torrent->infohash[i] & 0xFF);
  }
  printf("\n\n");

  free(contents);
  free_torrent(torrent);

  printf("Done!\n");
  return 0;
}

int main() {
  unsigned int i;
  int result;

  for (i = 1; i <= TORRENT_COUNT; i++) {
    result = test_torrent(i);
    if (result != 0) {
      return result;
    }
  }
  return 0;
}
