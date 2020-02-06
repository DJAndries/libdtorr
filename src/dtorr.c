#include "dtorr/dtorr.h"
#include "log.h"
#include <stdlib.h>

dtorr_ctx* dtorr_init(dtorr_config* config) {
  dtorr_ctx* ctx = (dtorr_ctx*)malloc(sizeof(dtorr_ctx));
  if (ctx == 0) {
    return 0;
  }
  ctx->config = config;
  return ctx;
}


void dtorr_free(dtorr_ctx* ctx) {
  free(ctx);
}