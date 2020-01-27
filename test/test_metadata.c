#include <stdio.h>
#include <stdlib.h>
#include "dtorr/structs.h"
#include "dtorr/metadata.h"

int main() {
  dtorr_config* config = (dtorr_config*)malloc(sizeof(dtorr_config));
  dtorr_torrent* torrent;
  long size;
  unsigned long i;
  FILE* fp = fopen("test/torrents/1.torrent", "rb");
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

  config->log_level = 4;
  config->log_handler = 0;

  torrent = load_torrent_metadata(config, contents, (unsigned long)size);
  if (torrent == 0) {
    fprintf(stderr, "Failed to load torrent!\n");
    return 2;
  }

  printf("Torrent name: %s\n", torrent->name);
  printf("Announce URL: %s\n", torrent->announce);
  printf("Length: %lu\n", torrent->length);
  printf("Piece length: %lu\n", torrent->piece_length);
  printf("Piece count: %lu\n", torrent->piece_count);
  printf("Has files: %d\n", torrent->files != 0);
  printf("Hash: ");

  for (i = 0; i < 20; i++) {
    printf("%02X", torrent->infohash[i] & 0xFF);
  }
  printf("\n\n");

  printf("Done!\n");
  return 0;
}