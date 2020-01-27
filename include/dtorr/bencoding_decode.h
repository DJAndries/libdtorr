#ifndef BENCODING_DECODE_H
#define BENCODING_DECODE_H

#include "dtorr/structs.h"

dtorr_node* bencoding_decode(dtorr_config* config, char* value, unsigned long value_len);

#endif