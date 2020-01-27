#ifndef config_H
#define config_H

#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

#include "dtorr/structs.h"

void dlog(dtorr_config* config, int log_level, char* message, ...);

#endif