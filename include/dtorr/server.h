#ifndef SERVER_H
#define SERVER_H

#include "dtorr/structs.h"

int peer_server_start(dtorr_config* config);

int peer_server_accept(dtorr_config* config);

#endif
