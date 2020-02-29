#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include "dtorr/structs.h"

int peer_send_handshake(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer);

int peer_recv_handshake(dtorr_config* config, SOCKET s, char* ip, unsigned short port);

#endif
