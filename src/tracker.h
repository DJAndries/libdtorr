#ifndef TRACKER_H
#define TRACKER_H

#include "dtorr/structs.h"

int tracker_announce(dtorr_config* config, char* tracker_uri, dtorr_torrent* torrent);

#endif