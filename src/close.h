#ifndef CLOSE_H
#define CLOSE_H

#include "dtorr/structs.h"

void peer_close(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, char bad);

#endif
