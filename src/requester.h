#ifndef REQUESTER_H
#define REQUESTER_H

#include "dtorr/structs.h"

void interest_update(dtorr_config* config, dtorr_torrent* torrent);

int send_requests(dtorr_config* config, dtorr_torrent* torrent);

#endif