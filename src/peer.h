#ifndef PEER_H
#define PEER_H

#include "dtorr/structs.h"

dtorr_peer* peer_get_or_create(dtorr_config* config, dtorr_torrent* torrent, char* ip, unsigned short port);

int peer_init_bitfield(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer);

#endif
