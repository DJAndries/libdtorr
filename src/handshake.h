#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include "dtorr/structs.h"

int peer_handshake(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer);

#endif