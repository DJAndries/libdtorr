#include <stdio.h>
#include <string.h>
#include "dtorr/structs.h"
#include "dtorr/metadata.h"
#include "state_persist.h"
#include "dsock.h"
#include "tracker.h"

dtorr_torrent* load_torrent(dtorr_config* config, char* filename) {
  dtorr_torrent* torrent;
  long size;
  FILE* fp = fopen(filename, "rb");
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

int persist_state(dtorr_config* config, dtorr_torrent* torrent, char* filename) {
  FILE* fp;
  unsigned long state_len;
  char* saved_state = save_state(config, torrent, &state_len);
  if (saved_state == 0) {
    return 1;
  }
  fp = fopen(filename, "wb");
  fwrite(saved_state, 1, state_len, fp);
  fclose(fp);
  return 0;
}

int main(int argc, char** argv) {
  dtorr_config config;
  dtorr_torrent *torrent, *torrent_with_state;
  unsigned long i;

  if (argc < 2) {
    printf("Must specify download dir\n");
    return 1;
  }

  if (dsock_init() != 0) {
    printf("Unable to init socket.\n");
    return 1;
  }

  config.log_level = 4;
  config.log_handler = 0;

  torrent = load_torrent(&config, "test/torrents/1.torrent");
  if (torrent == 0) {
    return 2;
  }
  torrent->download_dir = argv[1];
  torrent->bitfield[2] = torrent->bitfield[10] = torrent->bitfield[20] = 1;
  if (persist_state(&config, torrent, "test/torrents/test_state.torrent") != 0) {
    return 3;
  }

  torrent_with_state = load_torrent(&config, "test/torrents/test_state.torrent");
  if (torrent_with_state == 0) {
    return 4;
  }

  printf("Download dir: %s\n", torrent_with_state->download_dir);
  printf("Bitfield: ");
  for (i = 0; i < torrent_with_state->piece_count; i++) {
    printf("%d", torrent_with_state->bitfield[i]);
  }
  printf("\n");

  dsock_clean();

  printf("\nDone!\n");
  return 1;
}
