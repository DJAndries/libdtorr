#include <stdio.h>
#include "dtorr/structs.h"
#include "dtorr/metadata.h"
#include "dsock.h"
#include "tracker.h"
#include "manager.h"
#include "util.h"
#include "fs.h"

dtorr_torrent* load_torrent(dtorr_config* config) {
  dtorr_torrent* torrent;
  long size;
  FILE* fp = fopen("test/torrents/1.torrent", "rb");
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
  unsigned long i;
  char* bitfield;

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

  torrent = load_torrent(&config);
  if (torrent == 0) {
    return 2;
  }

  torrent->downloaded = 1;

  sprintf(torrent->me.ip, "192.168.0.1");
  torrent->me.port = 300;
  memcpy(torrent->me.peer_id, "14366678981935567890", 20);
  torrent->download_dir = argv[1];
  if (init_torrent_files(&config, torrent) != 0) {
    return 1;
  }

  if (tracker_announce(&config, torrent->announce, torrent) != 0) {
    return 3;
  }

  for (i = 0; i < 80; i++) {
    if (manage_torrent(&config, torrent) != 0) {
      return 1;
    }
    dsleep(500);
  }

  if (torrent->active_peers == 0) {
    printf("No active peers!");
    return 2;
  }

  bitfield = ((dtorr_peer*)torrent->active_peers->value)->bitfield;
  printf("Bitfield: ");
  for (i = 0; i < torrent->piece_count; i++) {
    printf("%d", bitfield[i]);
  }

  dsock_clean();

  printf("\nDone!\n");
  return 1;
}