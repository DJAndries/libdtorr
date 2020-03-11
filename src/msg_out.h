#ifndef MSG_OUT_H
#define MSG_OUT_H

#include "dtorr/structs.h"

int send_bitfield(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer);

int send_interested_status(dtorr_config* config, dtorr_peer* peer, char interested);

int send_choked_status(dtorr_config* config, dtorr_peer* peer, char choked);

int send_have(dtorr_config* config, dtorr_torrent* torrent, unsigned long long index);

int send_request(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, dtorr_piece_request* req);

int send_piece(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,dtorr_piece_request* req, char* data);

#endif
