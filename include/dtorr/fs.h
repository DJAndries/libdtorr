#ifndef FS_H
#define FS_H

#include "dtorr/structs.h"

int init_torrent_files(dtorr_config* config, dtorr_torrent* torrent);

int rw_piece(dtorr_config* config, dtorr_torrent* torrent, unsigned long index, char* buf, unsigned long buf_size, char is_write);

#endif
