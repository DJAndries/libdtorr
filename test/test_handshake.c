#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dtorr/structs.h"
#include "dtorr/metadata.h"
#include "dsock.h"
#include "handshake.h"
#include "util.h"

dtorr_torrent* load_torrent(dtorr_config* config) {
  dtorr_torrent* torrent;
  long long size;
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

  torrent = load_torrent_metadata(config, contents, (unsigned long long)size);
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
  dtorr_peer* target;
  unsigned long long i;

  if (argc < 3) {
    printf("Must specify target ip and port\n");
    return 1;
  }

  if (dsock_init() != 0) {
    printf("Unable to init socket.\n");
    return 2;
  }

  config.log_level = 4;
  config.log_handler = 0;

  target = (dtorr_peer*)malloc(sizeof(dtorr_peer));
  memset(target, 0, sizeof(dtorr_peer));
  strcpy(target->ip, argv[1]);
  target->port = strtoul(argv[2], 0, 0);

  torrent = load_torrent(&config);
  if (torrent == 0) {
    return 1;
  }
  torrent->downloaded = 1;

  sprintf(torrent->me.ip, "192.168.0.1");
  torrent->me.port = 300;
  memcpy(torrent->me.peer_id, "14366678981935567890", 20);

  if (peer_send_handshake(&config, torrent, target) != 0) {
    return 1;
  }

  if (torrent->active_peers == 0) {
    return 2;
  }
  printf("Active peer id: ");
  for (i = 0; i < 20; i++) {
    printf("%02X", ((dtorr_peer*)(torrent->active_peers->value))->peer_id[i]);
  }
  printf("\n");

  dsleep(10000);

  dsock_clean();

  printf("Done!\n");
  return 0;
}
