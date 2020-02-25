#include <stdio.h>
#include "dtorr/structs.h"
#include "dtorr/metadata.h"
#include "tracker.h"
#include "dsock.h"
#include "hashmap.h"

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
  dtorr_hashnode** peers;
  dtorr_hashnode** intervals;
  unsigned long i;

  if (argc < 2) {
    printf("Must specify tracker URL\n");
    return 1;
  }

  if (dsock_init() != 0) {
    printf("Unable to init socket.\n");
    return 2;
  }

  config.log_level = 1;
  config.log_handler = 0;

  torrent = load_torrent(&config);
  if (torrent == 0) {
    return 1;
  }
  torrent->downloaded = 1;

  sprintf(torrent->me.ip, "192.168.0.1");
  torrent->me.port = 300;
  memcpy(torrent->me.peer_id, "14366678981935567890", 20);

  if (tracker_announce(&config, argv[1], torrent) != 0) {
    return 3;
  }

  dsock_clean();

  peers = hashmap_entries(torrent->peer_map, 0);
  printf("Peers: \n");
  for (i = 0; i < torrent->peer_map->entry_count; i++) {
    printf("%s: %s:%u\n", peers[i]->key, ((dtorr_peer*)peers[i]->value)->ip, ((dtorr_peer*)peers[i]->value)->port);
  }
  printf("\nIntervals:\n");
  intervals = hashmap_entries(torrent->tracker_interval_map, 0);
  for (i = 0; i < torrent->tracker_interval_map->entry_count; i++) {
    printf("%s: %lu seconds\n", intervals[i]->key, *((long*)intervals[i]->value));
  }

  printf("Done!\n");
  return 0;
}
