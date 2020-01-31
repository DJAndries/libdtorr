#include <stdio.h>
#include <stdlib.h>
#include "dtorr/structs.h"
#include "dtorr/metadata.h"
#include "fs.h"

dtorr_torrent* load_torrent(dtorr_config* config) {
  dtorr_torrent* torrent;
  long size;
  FILE* fp = fopen("test/torrents/2.torrent", "rb");
  char* contents;

  if (fp == 0) {
    fprintf(stderr, "Could not open torrent file\n");
    return 0;
  }

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  contents = (char*)malloc(size + 1);
  fread(contents, 1, size, fp);
  fclose(fp);

  contents[size] = 0;

  torrent = load_torrent_metadata(config, contents, (unsigned long)size);
  if (torrent == 0) {
    fprintf(stderr, "Failed to load torrent!\n");
    return 0;
  }

  free(contents);

  return torrent;
}

int main(int argc, char** argv) {
  dtorr_config config;
  dtorr_torrent* torrent;

  if (argc < 2) {
    printf("Must specify download dir\n");
    return 1;
  }

  config.log_level = 1;
  config.log_handler = 0;

  torrent = load_torrent(&config);
  if (torrent == 0) {
    return 1;
  }
  torrent->download_dir = argv[1];
  if (init_torrent_files(&config, torrent) != 0) {
    return 1;
  }

  printf("Done!\n");
  return 0;
}