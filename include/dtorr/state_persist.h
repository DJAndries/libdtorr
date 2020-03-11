#ifndef STATE_PERSIST_H
#define STATE_PERSIST_H

#include "dtorr/structs.h"

char* save_state(dtorr_config* config, dtorr_torrent* torrent, unsigned long long* result_len);
int parse_state(dtorr_config* config, dtorr_torrent* torrent);

#endif
