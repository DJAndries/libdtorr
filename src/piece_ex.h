#ifndef PIECE_EXCHANGE_H
#define PIECE_EXCHANGE_H

#include "dtorr/structs.h"

int piece_recv(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  unsigned long index, unsigned long begin, char* piece, unsigned long piece_len);

int queue_request(dtorr_config* config, dtorr_peer* peer, unsigned long index,
                  unsigned long begin, unsigned long length);

int pieces_send(dtorr_config* config, dtorr_torrent* torrent);

#endif
