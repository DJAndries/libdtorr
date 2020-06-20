#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "dtorr/structs.h"
#include "dtorr/metadata.h"
#include "dtorr/server.h"
#include "dsock.h"
#include "dtorr/manager.h"
#include "dtorr/state_persist.h"
#include "util.h"
#include "dtorr/fs.h"

volatile sig_atomic_t interrupted = 0;
dtorr_torrent* torrent;

void intr_func(int sig) {
  interrupted = 1;
}

dtorr_torrent* load_torrent(dtorr_config* config, char* filename) {
  dtorr_torrent* result;
  long long size;
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

  result = load_torrent_metadata(config, contents, (unsigned long long)size);
  if (result == 0) {
    fprintf(stderr, "Failed to load torrent!\n");
    return 0;
  }

  free(contents);

  return result;
}

int persist_state(dtorr_config* config, dtorr_torrent* torrent, char* filename) {
  FILE* fp;
  unsigned long long state_len;
  char* saved_state = save_state(config, torrent, &state_len);
  if (saved_state == 0) {
    return 1;
  }
  fp = fopen(filename, "wb");
  fwrite(saved_state, 1, state_len, fp);
  fclose(fp);
  return 0;
}

dtorr_torrent* torrent_lookup(char* infohash) {
  if (memcmp(infohash, torrent->infohash, 20) == 0) {
    return torrent;
  }
  return 0;
}

int main(int argc, char** argv) {
  dtorr_config config;
  unsigned long long i;
  char* bitfield;

  if (argc < 5) {
    printf("Must specify download dir, torrent, torrent save location, port\n");
    return 1;
  }

  if (dsock_init() != 0) {
    printf("Unable to init socket.\n");
    return 1;
  }

  config.log_level = 4;
  config.log_handler = 0;
  sscanf(argv[4], "%hu", &config.port);
  config.torrent_lookup = torrent_lookup;

  torrent = load_torrent(&config, argv[2]);
  if (torrent == 0) {
    return 2;
  }

  torrent->downloaded = 1;

  sprintf(torrent->me.ip, "192.168.0.1");
  torrent->me.port = config.port;
  memcpy(torrent->me.peer_id, "14366678981935567890", 20);
  torrent->download_dir = argv[1];
  if (torrent->bitfield[0] == 0) {
    if (init_torrent_files(&config, torrent) != 0) {
      return 1;
    }
  }

  if (peer_server_start(&config) != 0) {
    return 1;
  }

  signal(SIGINT, intr_func);

  while (interrupted == 0) {
    peer_server_accept(&config);
    if (manage_torrent(&config, torrent) != 0) {
      return 1;
    }
    // dsleep(500);
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

  persist_state(&config, torrent, argv[3]);

  dsock_clean();

  printf("\nDone!\n");
  return 1;
}
