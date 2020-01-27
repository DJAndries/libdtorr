#ifndef DTORR_H
#define DTORR_H

#include "structs.h"

dtorr_ctx* dtorr_init(dtorr_config* config);

void dtorr_free(dtorr_ctx* ctx);

#endif