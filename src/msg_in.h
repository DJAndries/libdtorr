#ifndef MSG_IN_H
#define MSG_IN_H

#include "dtorr/structs.h"

int process_msg(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  char* in, unsigned long long in_len);

#endif
