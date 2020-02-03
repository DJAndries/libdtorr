#ifndef PIECE_EXCHANGE_H
#define PIECE_EXCHANGE_H

#include "dtorr/structs.h"

int piece_recv(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  unsigned long index, unsigned long begin, char* piece, unsigned long piece_len);

#endif