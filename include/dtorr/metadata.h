#ifndef METADATA_H
#define METADATA_H

#include "structs.h"

dtorr_torrent* load_torrent_metadata(dtorr_config* config, char* data, unsigned long long data_len);

void free_torrent(dtorr_torrent* torrent);

#endif
